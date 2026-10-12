#include "tag_peer.hpp"

#include "arena.hpp"

namespace {
constexpr double GRAVITY = 0.002;  // pixels per Timeline unit squared
constexpr int CONTROL_SPEED = 3;
}  // namespace

TagPeer::TagPeer(const TagConfig& config)
    : config_(config),
      timeline_([this] { return nowNs_; }),
      game_(CONTROL_SPEED, GRAVITY, config.windowWidth, config.windowHeight, config.peerId, false,
            config.players) {
    game_.setTimeline(timeline_);
    game_.setEventManager(events_);
    events_.registerHandler(EventType::COLLISION, &collisionHandler_);
    events_.registerHandler(EventType::CONTROL, &controlHandler_);
    events_.registerHandler(EventType::DEATH, &deathHandler_);
    events_.registerHandler(EventType::SIDEBOUNDARY, &sideBoundaryHandler_);

    arena::build(game_);
    arena::addPlayer(game_, config_.peerId, true);
}

void TagPeer::tick(const Keys& keys, int64_t elapsedNs) {
    nowNs_ += elapsedNs;
    handlePeerKeys(keys);
    game_.setInput(keys);
    game_.update();
    previousKeys_ = keys;
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
