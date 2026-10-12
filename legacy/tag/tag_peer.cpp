#include "tag_peer.hpp"

#include "arena.hpp"

namespace {
constexpr double GRAVITY = 0.5;
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
    game_.setInput(keys);
    game_.update();
}

std::shared_ptr<Entity> TagPeer::localPlayer() {
    return game_.getEntity(arena::playerEntityId(config_.peerId));
}
