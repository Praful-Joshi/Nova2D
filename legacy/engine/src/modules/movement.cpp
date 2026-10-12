#include "movement.hpp"

void MovementAttribute::update(Entity& entity, Game& game) {
    const double delta_time = entity.getDeltaTime();

    // Platforms follow their own path
    auto* platform = dynamic_cast<Platform*>(&entity);
    if (platform) {
        platform->update(delta_time);
        return;
    }

    if (!entity.getisMovable()) return;

    // Vertical movement here only for entities without gravity; gravity integrates its own
    const double dy = entity.getHasGravity() ? 0.0 : entity.getVelocityY() * delta_time;
    entity.moveBy(entity.getVelocityX() * delta_time, dy);
}
