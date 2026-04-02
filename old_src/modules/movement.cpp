#include "movement.hpp"

#include <iostream>
void MovementAttribute::update(Entity& entity, Game& game) {
    Timeline* timeline = game.getTimeline();
    int64_t elapsed_time = timeline->getTime();
    static int64_t last_time = elapsed_time;

    // Calculate delta time since last frame
    int64_t delta_time = elapsed_time - last_time;
    last_time = elapsed_time;

    // Check if the entity is a platform
    auto* platform = dynamic_cast<Platform*>(&entity);
    if (platform) {
        platform->update(delta_time);
        return;  // Skip further processing for platforms
    }

    // Skip entities without movement
    if ((entity.getVelocityX() == 0.0 && entity.getVelocityY() == 0.0 && !entity.getHasGravity()) ||
        !entity.getisMovable()) {
        return;
    }

    // Horizontal movement
    if (entity.getVelocityX() != 0) {
        if (timeline->isPaused()) {
            entity.setRectX(entity.getRect().x);
        } else {
            entity.setRectX((int)(entity.getRect().x + entity.getVelocityX() * (double)delta_time));
        }
    }

    // Vertical movement only if the entity does not have gravity
    if (!entity.getHasGravity() && entity.getVelocityY() != 0) {
        entity.setRectY((int)(entity.getRect().y + entity.getVelocityY() * (double)delta_time));
    }
}
