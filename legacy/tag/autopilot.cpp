#include "autopilot.hpp"

#include <cmath>
#include <limits>

#include "arena.hpp"

Keys Autopilot::keys(TagPeer& peer) {
    auto me = peer.localPlayer();
    if (!me || !me->isActive()) return Keys{};

    const int itId = peer.itPeerId();
    const bool iAmIt = itId == peer.peerId();
    const double myX = me->getX();

    // Who to chase or run from
    std::shared_ptr<Entity> other;
    double nearest = std::numeric_limits<double>::max();
    for (int id = 1; id <= arena::MAX_PLAYERS; ++id) {
        if (id == peer.peerId()) continue;
        if (!iAmIt && id != itId) continue;
        auto player = peer.game().getEntity(arena::playerEntityId(id));
        if (!player || !player->isActive()) continue;
        const double distance = std::abs(player->getX() - myX);
        if (distance < nearest) {
            nearest = distance;
            other = player;
        }
    }

    std::uniform_int_distribution<int> percent(0, 99);
    if (other) {
        const int towards = other->getX() > myX ? 1 : -1;
        direction_ = iAmIt ? towards : -towards;
        // A runner cornered at an arena end doubles back past It
        if (!iAmIt && (myX < 60 || myX > arena::WIDTH - 100)) direction_ = -direction_;
    } else if (percent(random_) < 1) {
        direction_ = -direction_;  // wander in the Lobby
    }

    stuckTicks_ = std::abs(myX - lastX_) < 0.5 ? stuckTicks_ + 1 : 0;
    lastX_ = myX;
    const bool jump = stuckTicks_ > 6 || percent(random_) < 2 ||
                      (iAmIt && other && other->getY() < me->getY() - 60);

    Keys keys;
    keys.set(direction_ > 0 ? SDL_SCANCODE_RIGHT : SDL_SCANCODE_LEFT, true);
    keys.set(SDL_SCANCODE_UP, jump);
    return keys;
}
