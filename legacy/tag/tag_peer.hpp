#ifndef TAG_PEER_HPP
#define TAG_PEER_HPP

#include <cstdint>
#include <memory>

#include "collision_handler.hpp"
#include "control_handler.hpp"
#include "death_handler.hpp"
#include "eventmanager.hpp"
#include "game.hpp"
#include "input.hpp"
#include "sideboundary_handler.hpp"
#include "timeline.hpp"

struct TagConfig {
    int peerId = 1;
    int players = 1;
    int windowWidth = 1280;
    int windowHeight = 720;
};

// One running instance of Tag. nova2d_tag drives it from SDL and the real clock;
// tests drive it headlessly by passing the held keys and advancing time in fixed ticks.
class TagPeer {
public:
    static constexpr int64_t TICK_NS = 1'000'000'000 / 60;

    explicit TagPeer(const TagConfig& config);
    TagPeer(const TagPeer&) = delete;
    TagPeer& operator=(const TagPeer&) = delete;

    // Advances real time by elapsedNs and runs one frame with the given keys held
    void tick(const Keys& keys, int64_t elapsedNs = TICK_NS);

    Game& game() { return game_; }
    Timeline& timeline() { return timeline_; }
    [[nodiscard]] int peerId() const { return config_.peerId; }
    std::shared_ptr<Entity> localPlayer();

private:
    TagConfig config_;
    int64_t nowNs_ = 0;
    Timeline timeline_;
    EventManager events_;
    CollisionHandler collisionHandler_;
    ControlHandler controlHandler_;
    DeathHandler deathHandler_;
    SideBoundaryHandler sideBoundaryHandler_;
    Game game_;
};

#endif
