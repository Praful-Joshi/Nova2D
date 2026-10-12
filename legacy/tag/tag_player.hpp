#ifndef TAG_PLAYER_HPP
#define TAG_PLAYER_HPP

#include <cstdint>

#include "attribute.hpp"

// Tag's own Attribute on each player Entity (Property::Custom): the player's part in the
// current Round, as this Peer sees it. Times are this Peer's Timeline time in ms.
class TagPlayerAttribute : public Attribute {
public:
    void update(Entity& /*entity*/, Game& /*game*/) override {}

    [[nodiscard]] bool isFrozen(int64_t now) const { return now < frozenUntil; }
    [[nodiscard]] bool isImmune(int64_t now) const { return now < immuneUntil; }

    // Watching this Round instead of playing it
    bool spectator = false;
    // Chasing the others
    bool it = false;
    // It can't move until then: a head start for the others
    int64_t frozenUntil = 0;
    // Has Tag-back Immunity against It until then, having just tagged them
    int64_t immuneUntil = 0;
    // Time spent as It while the Round was Playing
    int64_t itTimeMs = 0;
};

#endif
