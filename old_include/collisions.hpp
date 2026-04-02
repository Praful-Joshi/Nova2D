#ifndef COLLISION_ATTRIBUTE_HPP
#define COLLISION_ATTRIBUTE_HPP

#include "attribute.hpp"
#include "eventmanager.hpp"
#include "game.hpp"

class CollisionAttribute : public Attribute {
public:
    void update(Entity& entity, Game& game) override;
};

#endif  // COLLISION_ATTRIBUTE_HPP
