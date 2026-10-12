#include "tag_peer.hpp"

#include <algorithm>
#include <utility>

#include "arena.hpp"
#include "control.hpp"
#include "platform.hpp"
#include "tag_player.hpp"

namespace {
constexpr double GRAVITY = 0.002;  // pixels per Timeline unit squared
constexpr int CONTROL_SPEED = 3;
}  // namespace

std::string TagConfig::peerAddress(int otherPeerId) const {
    auto it = peerAddresses.find(otherPeerId);
    return it != peerAddresses.end() ? it->second : "localhost";
}

TagPeer::TagPeer(const TagConfig& config) : TagPeer(config, nullptr) {}

TagPeer::TagPeer(const TagConfig& config, std::shared_ptr<Transport> transport)
    : config_(config),
      timeline_([this] { return nowNs_; }),
      game_(CONTROL_SPEED, GRAVITY, config.windowWidth, config.windowHeight, config.peerId, false,
            config.players),
      network_(config.peerId, std::move(transport)),
      rounds_(timeline_),
      roundHandler_(*this) {
    game_.setTimeline(timeline_);
    game_.setEventManager(events_);
    events_.registerHandler(EventType::COLLISION, &collisionHandler_);
    events_.registerHandler(EventType::CONTROL, &controlHandler_);
    events_.registerHandler(EventType::DEATH, &deathHandler_);
    events_.registerHandler(EventType::SIDEBOUNDARY, &sideBoundaryHandler_);
    events_.registerHandler(ROUND_START, &roundHandler_);
    events_.registerHandler(ROUND_END, &roundHandler_);

    arena::build(game_);
    arena::addPlayer(game_, config_.peerId, true);
}

void TagPeer::tick(const Keys& keys, int64_t elapsedNs) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    nowNs_ += elapsedNs;
    for (const Message& message : network_.receive(game_, nowNs_)) handleMessage(message);
    // Once everyone expected has joined, send only changed state. It stays on if a Peer
    // leaves; a Peer that (re)joins is sent full state when first heard.
    if (static_cast<int>(presentPeers().size()) >= config_.players) {
        network_.setChangeOnly(true);
    }
    handlePeerKeys(keys);
    if (isHost()) runHost();
    events_.processEvents();  // Round changes take effect before this frame's physics
    rounds_.advance();
    applyRoles();

    game_.setInput(keys);
    game_.update();
    network_.interpolate(game_, nowNs_);
    network_.sendDue(localPlayer().get(), nowNs_);
    if (isHost()) sendHostStateDue();
    previousKeys_ = keys;
}

std::vector<int> TagPeer::presentPeers() const {
    std::vector<int> ids = network_.connectedPeers();
    ids.push_back(config_.peerId);
    std::sort(ids.begin(), ids.end());
    return ids;
}

void TagPeer::leave() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    network_.leave();
}

void TagPeer::requestFullState() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    network_.forceFullState();
}

void TagPeer::handleMessage(const Message& message) {
    // Only the Host has authority over shared state
    if (message.sender != hostId_) return;

    if (const auto* state = std::get_if<HostState>(&message.body)) {
        followHost(*state);
    } else if (const auto* start = std::get_if<RoundStart>(&message.body)) {
        if (rounds_.compare(start->round, RoundPhase::COUNTDOWN) > 0) {
            raiseRoundEvent(ROUND_START, start->round, start->participants);
        }
    } else if (const auto* end = std::get_if<RoundEnd>(&message.body)) {
        if (rounds_.compare(end->round, RoundPhase::RESULTS) > 0) {
            raiseRoundEvent(ROUND_END, end->round, rounds_.participants());
        }
    }
}

void TagPeer::runHost() {
    // A Round needs at least two players; a lone Peer practises in the Lobby
    const std::vector<int> present = presentPeers();
    const bool enoughPlayers =
        static_cast<int>(present.size()) >= std::max(2, config_.players);

    switch (rounds_.phase()) {
        case RoundPhase::LOBBY:
            if (enoughPlayers) startRound(rounds_.round() + 1, present);
            break;
        case RoundPhase::COUNTDOWN:
            break;  // every Peer runs the Countdown's clock itself
        case RoundPhase::PLAYING:
            if (rounds_.isTimeUp()) endRound();
            break;
        case RoundPhase::RESULTS:
            if (rounds_.isTimeUp()) {
                if (present.size() >= 2) {
                    startRound(rounds_.round() + 1, present);
                } else {
                    rounds_.enter(RoundPhase::LOBBY, rounds_.round(), {}, 0);
                }
            }
            break;
    }
}

void TagPeer::startRound(int round, const std::vector<int>& participants) {
    raiseRoundEvent(ROUND_START, round, participants);
    network_.send(RoundStart{round, participants});
}

void TagPeer::endRound() {
    raiseRoundEvent(ROUND_END, rounds_.round(), rounds_.participants());
    network_.send(RoundEnd{rounds_.round()});
}

void TagPeer::sendHostStateDue() {
    if (nowNs_ < nextHostStateNs_) return;
    nextHostStateNs_ = nowNs_ + TagNetwork::STATE_INTERVAL_NS;

    HostState state;
    state.phase = rounds_.phase();
    state.remainingMs = rounds_.remainingMs();
    state.round = rounds_.round();
    state.participants = rounds_.participants();
    for (const auto& [id, entity] : game_.getEntities()) {
        if (auto* platform = dynamic_cast<Platform*>(entity.get())) {
            state.platforms.push_back(PlatformState{id, platform->getX(), platform->getY(),
                                                    platform->getDirectionX(),
                                                    platform->getDirectionY()});
        }
    }
    std::sort(state.platforms.begin(), state.platforms.end(),
              [](const PlatformState& a, const PlatformState& b) { return a.id < b.id; });
    network_.sendState(state);
}

void TagPeer::followHost(const HostState& state) {
    for (const PlatformState& platformState : state.platforms) {
        auto platform = std::dynamic_pointer_cast<Platform>(game_.getEntity(platformState.id));
        if (!platform) continue;
        platform->setPosition(platformState.x, platformState.y);
        platform->setDirection(platformState.directionX, platformState.directionY);
    }

    const int order = rounds_.compare(state.round, state.phase);
    if (order > 0) {
        // Catching up, e.g. after joining mid-Round
        const bool newRound = state.round != rounds_.round();
        rounds_.enter(state.phase, state.round, state.participants, state.remainingMs);
        if (newRound && !rounds_.isSpectator(config_.peerId)) placeAtSpawn();
    } else if (order == 0 && state.phase != RoundPhase::LOBBY) {
        rounds_.setRemainingMs(state.remainingMs);
    }
}

void TagPeer::raiseRoundEvent(const char* name, int round, const std::vector<int>& participants) {
    Event event = Event::custom(name, EventContext{&game_, nullptr}, 0,
                                static_cast<unsigned long long>(timeline_.getTime()));
    event.set("round", round);
    event.set("participants", joinPeerIds(participants));
    events_.raise(event);
}

void TagPeer::onRoundEvent(const Event& event) {
    const int round = event.getInt("round");
    const std::vector<int> participants = splitPeerIds(event.getString("participants"));
    if (event.name() == ROUND_START) {
        rounds_.enter(RoundPhase::COUNTDOWN, round, participants, Rounds::COUNTDOWN_MS);
        if (!rounds_.isSpectator(config_.peerId)) placeAtSpawn();
        network_.forceFullState();  // so Spectators and new players have complete state
    } else if (event.name() == ROUND_END) {
        rounds_.enter(RoundPhase::RESULTS, round, participants, Rounds::RESULTS_MS);
    }
}

void TagPeer::placeAtSpawn() {
    auto player = localPlayer();
    auto spawn = game_.getEntity(arena::spawnPointId(config_.peerId));
    if (!player || !spawn) return;
    player->setPosition(spawn->getX(), spawn->getY());
    player->setVelocityX(0);
    player->setVelocityY(0);
    player->setGrounded(false);
    player->setActive(true);
}

void TagPeer::applyRoles() {
    for (int peerId = 1; peerId <= arena::MAX_PLAYERS; ++peerId) {
        auto player = game_.getEntity(arena::playerEntityId(peerId));
        if (!player) continue;
        const bool spectator = rounds_.isSpectator(peerId);
        auto tag = game_.getAttribute<TagPlayerAttribute>(Property::Custom, player->getEntityID());
        if (tag) tag->spectator = spectator;
        if (peerId != config_.peerId) continue;

        // Only the local player is ours to change; others follow their own Peer's state
        if (spectator) player->setActive(false);
        if (auto control = game_.getAttribute<ControlAttribute>(Property::Control,
                                                                 player->getEntityID())) {
            control->setFrozen(spectator || rounds_.phase() == RoundPhase::COUNTDOWN);
        }
    }
}

bool TagPeer::pressed(const Keys& keys, SDL_Scancode key) const {
    return keys.held(key) && !previousKeys_.held(key);
}

void TagPeer::handlePeerKeys(const Keys& keys) {
    if (pressed(keys, SDL_SCANCODE_T)) game_.toggleScalingMode();

    if (pressed(keys, SDL_SCANCODE_P)) {
        if (timeline_.isPaused()) {
            timeline_.unpause();
        } else {
            timeline_.pause();
        }
    }
    if (pressed(keys, SDL_SCANCODE_1)) timeline_.changeTic(500'000);    // 2x speed
    if (pressed(keys, SDL_SCANCODE_2)) timeline_.changeTic(1'000'000);  // normal speed
    if (pressed(keys, SDL_SCANCODE_3)) timeline_.changeTic(2'000'000);  // 0.5x speed
}

std::shared_ptr<Entity> TagPeer::localPlayer() {
    return game_.getEntity(arena::playerEntityId(config_.peerId));
}
