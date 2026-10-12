#ifndef TAG_PLAYER_HPP
#define TAG_PLAYER_HPP

#include "attribute.hpp"

// Tag's own Attribute on each player Entity (Property::Custom): the player's part in the
// current Round, as this Peer sees it
class TagPlayerAttribute : public Attribute {
public:
    void update(Entity& /*entity*/, Game& /*game*/) override {}

    // Watching this Round instead of playing it
    bool spectator = false;
};

#endif
