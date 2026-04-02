// spawn_point_core/attribute.hpp
#ifndef SPAWN_POINT_ATTRIBUTE_HPP
#define SPAWN_POINT_ATTRIBUTE_HPP

#include "attribute.hpp"
#include "entity.hpp"
#include "game.hpp"

class SpawnPointAttribute : public Attribute {
public:
    void update(Entity& entity, Game& game) override;
};

#endif  // SPAWN_POINT_ATTRIBUTE_HPP
