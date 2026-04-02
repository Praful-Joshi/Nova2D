// gravity_core/attribute.hpp
#ifndef GRAVITY_ATTRIBUTE_HPP
#define GRAVITY_ATTRIBUTE_HPP

#include "attribute.hpp"
#include "entity.hpp"
#include "game.hpp"

class GravityAttribute : public Attribute {
public:
    void update(Entity& entity, Game& game) override;
};

#endif  // GRAVITY_ATTRIBUTE_HPP
