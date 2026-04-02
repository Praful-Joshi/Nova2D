#ifndef CONTROL_ATTRIBUTE_HPP
#define CONTROL_ATTRIBUTE_HPP

#include "attribute.hpp"
#include "eventmanager.hpp"
#include "game.hpp"

class ControlAttribute : public Attribute {
public:
    void update(Entity& entity, Game& game) override;
};

#endif  // CONTROL_ATTRIBUTE_HPP
