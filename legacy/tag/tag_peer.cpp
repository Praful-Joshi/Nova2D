#include "tag_peer.hpp"

#include <algorithm>
#include <utility>

#include "arena.hpp"
#include "control.hpp"
#include "platform.hpp"
#include "tag_player.hpp"
#include "tagging.hpp"

namespace {
constexpr double GRAVITY = 0.002;  // pixels per Timeline unit squared
constexpr int CONTROL_SPEED = 3;

TimelineState timelineState(Timeline& timeline) {
    return TimelineState{timeline.isPaused(), timeline.getTic()};
}

void applyTimelineState(Timeline& timeline, const TimelineState& state) {
    if (timeline.getTic() != state.tic) timeline.changeTic(state.tic);
    if (state.paused) {
        timeline.pause();
    } else {
        timeline.unpause();
    }
}
}  // namespace

std::string TagConfig::peerAddress(int otherPeerId) const {
    auto it = peerAddresses.find(otherPeerId);
    return it != peerAddresses.end() ? it->second : "localhost";
}

TagPeer::TagPeer(const TagConfig& config) : TagPeer(config, nullptr) {}

TagPeer::TagPeer(const TagConfig& config, std::shared_ptr<Transport> transport)
    : config_(config),
      timeline_([this] { return nowNs_; }),
      game_(CONTROL_SPEED, GRAVITY, config.windowWidth, config.windowHeight, config.peerId),
      network_(config.peerId, std::move(transport)),
      rounds_(timeline_),
      roundHandler_(*this),
      random_(std::random_device{}()) {
    game_.setTimeline(timeline_);
    game_.setEventManager(events_);
    events_.registerHandler(EventType::COLLISION, &collisionHandler_);
    events_.registerHandler(EventType::CONTROL, &controlHandler_);
    events_.registerHandler(EventType::DEATH, &deathHandler_);
    events_.registerHandler(EventType::SIDEBOUNDARY, &sideBoundaryHandler_);
    events_.registerHandler(ROUND_START, &roundHandler_);
    events_.registerHandler(ROUND_END, &roundHandler_);
    events_.registerHandler(TAGGED, &roundHandler_);

    arena::build(game_);
    arena::addPlayer(game_, config_.peerId, true);
}

void TagPeer::tick(const Keys& keys, int64_t elapsedNs) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    nowNs_ += elapsedNs;
    const std::vector<Message> messages = network_.receive(game_, nowNs_);
    // The old Host's last messages count, then a new Host's ones that arrived alongside
    const int hostBefore = hostId_;
    for (const Message& message : messages) handleMessage(message);
    for (int departed : network_.departures()) onPeerDeparted(departed);
    if (hostId_ != hostBefore) {
        for (const Message& message : messages) {
            if (message.sender == hostId_) handleMessage(message);
        }
    }
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
    if (const auto* state = std::get_if<HostState>(&message.body)) {
        if (message.sender != hostId_ && isHostAhead(message.sender, *state)) {
            changeHost(message.sender);  // e.g. joining after Host Migration
        }
    }
    // Only the Host has authority over shared state
    if (message.sender != hostId_) return;
    hostHeard_ = true;

    if (const auto* state = std::get_if<HostState>(&message.body)) {
        followHost(*state);
    } else if (const auto* timeline = std::get_if<TimelineState>(&message.body)) {
        applyTimelineState(timeline_, *timeline);
    } else if (const auto* start = std::get_if<RoundStart>(&message.body)) {
        if (rounds_.compare(start->round, RoundPhase::COUNTDOWN) > 0) {
            raiseRoundEvent(ROUND_START, start->round, start->participants, start->it);
        }
    } else if (const auto* end = std::get_if<RoundEnd>(&message.body)) {
        if (rounds_.compare(end->round, RoundPhase::RESULTS) > 0) {
            tagging::setItTimes(game_, end->itTimes);  // the final totals
            raiseRoundEvent(ROUND_END, end->round, rounds_.participants());
        }
    } else if (const auto* tagged = std::get_if<Tagged>(&message.body)) {
        raiseTagged(tagged->from, tagged->to);
    }
}

void TagPeer::onPeerDeparted(int peerId) {
    if (peerId != hostId_) return;
    // Host Migration: a role change, continuing from the last HOST_STATE this Peer followed
    changeHost(presentPeers().front());
}

void TagPeer::changeHost(int newHostId) {
    if (newHostId == hostId_) return;
    hostId_ = newHostId;
    // Announce it only if this Peer was following another Host, not when it has just joined
    if (hostHeard_) hostChangedAtNs_ = nowNs_;
    hostHeard_ = isHost();
    network_.forceFullState();  // the new Host's full state, and everyone's player again
}

std::optional<int> TagPeer::hostChangeNotice() const {
    if (hostChangedAtNs_ < 0 || nowNs_ - hostChangedAtNs_ >= HOST_NOTICE_NS) return std::nullopt;
    return hostId_;
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
        case RoundPhase::PLAYING: {
            // Players may have left: a Round needs two, and someone must be It
            int remaining = 0;
            for (int id : rounds_.participants()) {
                if (std::binary_search(present.begin(), present.end(), id)) ++remaining;
            }
            if (remaining < 2) {
                endRound();
                break;
            }
            if (tagging::it(game_) == 0) reassignIt();
            // Every Peer runs the Countdown's clock itself
            if (rounds_.phase() == RoundPhase::PLAYING) {
                runTagging();
                if (rounds_.isTimeUp()) endRound();
            }
            break;
        }
        case RoundPhase::RESULTS:
            if (rounds_.isTimeUp()) {
                if (present.size() >= 2) {
                    startRound(rounds_.round() + 1, present);
                } else {
                    rounds_.enter(RoundPhase::LOBBY, rounds_.round(), {}, 0);
                    tagging::reset(game_);
                }
            }
            break;
    }
}

void TagPeer::startRound(int round, const std::vector<int>& participants) {
    const int it = pickRandom(participants);
    raiseRoundEvent(ROUND_START, round, participants, it);
    network_.send(RoundStart{round, participants, it});
}

bool TagPeer::isHostAhead(int sender, const HostState& state) const {
    // Another Peer is acting as Host. Follow it if its Round is further on than this Peer's;
    // when they're level, if this Peer can't hear its Host, or the lower Peer ID wins.
    const int order = rounds_.compare(state.round, state.phase);
    if (order != 0) return order > 0;
    const std::vector<int> present = presentPeers();
    const bool hostPresent = std::binary_search(present.begin(), present.end(), hostId_);
    return !hostPresent || sender < hostId_;
}

void TagPeer::endRound() {
    accountItTime();  // the Round may end early, between the Host's usual accounting
    raiseRoundEvent(ROUND_END, rounds_.round(), rounds_.participants());
    network_.send(RoundEnd{rounds_.round(), tagging::itTimes(game_, rounds_.participants())});
}

void TagPeer::runTagging() {
    accountItTime();
    const int it = tagging::it(game_);
    const int touched = tagging::findTagging(game_, rounds_.participants(), timeline_.getTime());
    if (touched == 0) return;
    raiseTagged(it, touched);
    network_.send(Tagged{it, touched});
}

void TagPeer::accountItTime() {
    if (rounds_.phase() != RoundPhase::PLAYING) return;
    // Counting Playing time already elapsed, so the totals add up to the Round's length
    const int64_t elapsedMs = Rounds::PLAYING_MS - rounds_.remainingMs();
    if (elapsedMs <= itAccountedMs_) return;
    if (auto it = tagging::playerTag(game_, tagging::it(game_))) {
        it->itTimeMs += elapsedMs - itAccountedMs_;
    }
    itAccountedMs_ = elapsedMs;
}

void TagPeer::raiseTagged(int from, int to) {
    Event event = Event::custom(TAGGED, EventContext{&game_, nullptr}, 0,
                                static_cast<unsigned long long>(timeline_.getTime()));
    event.set("from", from);
    event.set("to", to);
    events_.raise(event);
}

int TagPeer::pickRandom(const std::vector<int>& peerIds) {
    if (peerIds.empty()) return 0;
    std::uniform_int_distribution<std::size_t> index(0, peerIds.size() - 1);
    return peerIds[index(random_)];
}

void TagPeer::reassignIt() {
    const RoundPhase phase = rounds_.phase();
    if (!isHost() || (phase != RoundPhase::COUNTDOWN && phase != RoundPhase::PLAYING)) return;
    std::vector<int> remaining;
    for (int id : rounds_.participants()) {
        if (tagging::playerTag(game_, id)) remaining.push_back(id);
    }
    accountItTime();  // the time so far was the old It's
    // In the Countdown, the freeze starts when Playing does, as at the start of a Round
    const int64_t freezeFrom =
        timeline_.getTime() + (phase == RoundPhase::COUNTDOWN ? rounds_.remainingMs() : 0);
    tagging::makeIt(game_, pickRandom(remaining), freezeFrom + tagging::IT_FREEZE_MS);
}

int TagPeer::itPeerId() const { return tagging::it(game_); }

bool TagPeer::isFrozen(int peerId) {
    auto tag = tagging::playerTag(game_, peerId);
    return tag && tag->isFrozen(timeline_.getTime());
}

bool TagPeer::isImmune(int peerId) {
    auto tag = tagging::playerTag(game_, peerId);
    return tag && tag->isImmune(timeline_.getTime());
}

int64_t TagPeer::itTimeMs(int peerId) const {
    auto tag = tagging::playerTag(game_, peerId);
    return tag ? tag->itTimeMs : 0;
}

std::vector<ItTime> TagPeer::itTimes() const {
    return tagging::itTimes(game_, rounds_.participants());
}

std::vector<int> TagPeer::winners() const {
    return tagging::winners(game_, rounds_.participants());
}

void TagPeer::sendHostStateDue() {
    if (nowNs_ < nextHostStateNs_) return;
    nextHostStateNs_ = nowNs_ + TagNetwork::STATE_INTERVAL_NS;

    HostState state;
    state.phase = rounds_.phase();
    state.remainingMs = rounds_.remainingMs();
    state.round = rounds_.round();
    state.participants = rounds_.participants();
    state.timeline = timelineState(timeline_);
    for (const auto& [id, entity] : game_.getEntities()) {
        if (auto* platform = dynamic_cast<Platform*>(entity.get())) {
            state.platforms.push_back(PlatformState{id, platform->getX(), platform->getY(),
                                                    platform->getDirectionX(),
                                                    platform->getDirectionY()});
        }
    }
    std::sort(state.platforms.begin(), state.platforms.end(),
              [](const PlatformState& a, const PlatformState& b) { return a.id < b.id; });
    tagging::writeHostState(game_, rounds_.participants(), timeline_.getTime(), state);
    network_.sendState(state);
}

void TagPeer::followHost(const HostState& state) {
    applyTimelineState(timeline_, state.timeline);
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

    if (rounds_.compare(state.round, state.phase) == 0) {
        tagging::followHostState(game_, state, timeline_.getTime());
        // So that if this Peer becomes Host, It time carries on from the Host's totals
        if (state.phase == RoundPhase::PLAYING) {
            itAccountedMs_ = Rounds::PLAYING_MS - state.remainingMs;
        }
    }
}

void TagPeer::raiseRoundEvent(const char* name, int round, const std::vector<int>& participants,
                              int it) {
    Event event = Event::custom(name, EventContext{&game_, nullptr}, 0,
                                static_cast<unsigned long long>(timeline_.getTime()));
    event.set("round", round);
    event.set("participants", joinPeerIds(participants));
    event.set("it", it);
    events_.raise(event);
}

void TagPeer::onRoundEvent(const Event& event) {
    if (event.name() == TAGGED) {
        tagging::tag(game_, event.getInt("from"), event.getInt("to"),
                     timeline_.getTime() + tagging::IMMUNITY_MS);
        return;
    }

    const int round = event.getInt("round");
    const std::vector<int> participants = splitPeerIds(event.getString("participants"));
    if (event.name() == ROUND_START) {
        rounds_.enter(RoundPhase::COUNTDOWN, round, participants, Rounds::COUNTDOWN_MS);
        if (!rounds_.isSpectator(config_.peerId)) placeAtSpawn();
        // It's 2 s freeze starts when Playing does, giving the others a head start
        tagging::reset(game_);
        tagging::makeIt(game_, event.getInt("it"),
                        timeline_.getTime() + Rounds::COUNTDOWN_MS + tagging::IT_FREEZE_MS);
        itAccountedMs_ = 0;
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
            const int64_t now = timeline_.getTime();
            const bool frozen = spectator || rounds_.phase() == RoundPhase::COUNTDOWN ||
                                (tag && tag->isFrozen(now));
            control->setFrozen(frozen);
            // Changed only when the Round's block starts or ends, leaving other blocks alone
            const bool blocked = frozen || (tag && tag->isImmune(now));
            if (blocked != abilitiesBlockedByRoles_) control->setAbilitiesBlocked(blocked);
            abilitiesBlockedByRoles_ = blocked;
        }
    }
}

bool TagPeer::pressed(const Keys& keys, SDL_Scancode key) const {
    return keys.held(key) && !previousKeys_.held(key);
}

void TagPeer::handlePeerKeys(const Keys& keys) {
    if (pressed(keys, SDL_SCANCODE_T)) game_.toggleScalingMode();

    // Pause and speed are the Host's to set: it applies them and tells every Peer
    if (!isHost()) return;
    TimelineState state = timelineState(timeline_);
    if (pressed(keys, SDL_SCANCODE_P)) state.paused = !state.paused;
    if (pressed(keys, SDL_SCANCODE_1)) state.tic = 500'000;    // 2x speed
    if (pressed(keys, SDL_SCANCODE_2)) state.tic = 1'000'000;  // normal speed
    if (pressed(keys, SDL_SCANCODE_3)) state.tic = 2'000'000;  // 0.5x speed
    if (state == timelineState(timeline_)) return;
    applyTimelineState(timeline_, state);
    network_.send(state);
}

std::shared_ptr<Entity> TagPeer::localPlayer() {
    return game_.getEntity(arena::playerEntityId(config_.peerId));
}
