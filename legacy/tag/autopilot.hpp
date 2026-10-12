#ifndef TAG_AUTOPILOT_HPP
#define TAG_AUTOPILOT_HPP

#include <random>

#include "input.hpp"
#include "tag_peer.hpp"

// Plays Tag by itself, for demos and recordings (NOVA2D_TAG_AUTOPILOT): It chases the nearest
// player, everyone else runs from It, and players jump when blocked or now and then.
class Autopilot {
public:
    explicit Autopilot(unsigned seed) : random_(seed) {}

    // The keys to hold this tick
    Keys keys(TagPeer& peer);

private:
    std::mt19937 random_;
    double lastX_ = 0;
    int stuckTicks_ = 0;
    int direction_ = 1;
};

#endif
