#include "gravity.hpp"

#include <algorithm>

namespace {
// Fastest fall speed in pixels per Timeline unit, so a fall can't skip through thin platforms
constexpr double TERMINAL_VELOCITY = 0.9;
}  // namespace

void GravityAttribute::update(Entity& entity, Game& game) {
    const double dt = entity.getDeltaTime();
    const double g = game.getGravityStrength();
    const double v0 = entity.getVelocityY();

    // Exact constant-acceleration motion, so the path depends only on Timeline time
    double v1 = v0 + g * dt;
    double dy = v0 * dt + 0.5 * g * dt * dt;
    if (v1 > TERMINAL_VELOCITY) {
        v1 = TERMINAL_VELOCITY;
        if (v0 >= TERMINAL_VELOCITY) {
            dy = TERMINAL_VELOCITY * dt;
        } else {
            const double accelTime = (TERMINAL_VELOCITY - v0) / g;
            dy = v0 * accelTime + 0.5 * g * accelTime * accelTime +
                 TERMINAL_VELOCITY * (dt - accelTime);
        }
    }

    entity.setVelocityY(v1);
    entity.moveBy(0, dy);
    // Collision decides again whether the entity is standing on something
    entity.setGrounded(false);
}
