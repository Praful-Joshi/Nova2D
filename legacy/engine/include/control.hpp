#ifndef CONTROL_ATTRIBUTE_HPP
#define CONTROL_ATTRIBUTE_HPP

#include "attribute.hpp"
#include "eventmanager.hpp"
#include "game.hpp"

class ControlAttribute : public Attribute {
public:
    // Speeds are in pixels per Timeline unit
    explicit ControlAttribute(double runSpeed = 0.35, double jumpSpeed = 0.8)
        : runSpeed(runSpeed), jumpSpeed(jumpSpeed) {}

    void update(Entity& entity, Game& game) override;

    [[nodiscard]] double getRunSpeed() const { return runSpeed; }
    [[nodiscard]] double getJumpSpeed() const { return jumpSpeed; }

private:
    double runSpeed;
    double jumpSpeed;
};

#endif  // CONTROL_ATTRIBUTE_HPP
