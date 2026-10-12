#include "collision_handler.hpp"

#include <cmath>

void CollisionHandler::onEvent(const Event& event) {
    if (event.type == EventType::COLLISION) {
        updateGameState(*event.context.entity, *event.context.game);
    }
}

void CollisionHandler::updateGameState(Entity& entity, Game& game) {
    // Only entities that can be pushed get pushed out of solid (immovable) Collide entities.
    // Movable entities pass through each other, so players can touch.
    if (!entity.getisMovable()) return;

    for (const auto& [otherEntityId, otherEntityPtr] : game.getEntities()) {
        if (entity.getEntityID() == otherEntityId) continue;  // Skip self-collision
        if (otherEntityPtr->getisMovable()) continue;
        if (!game.entityHasProperty(Property::Collide, otherEntityId)) continue;
        resolveCollision(entity, *otherEntityPtr);
    }
}

void CollisionHandler::resolveCollision(Entity& entityA, Entity& entityB) {
    // Push movable entity A out of solid entity B along the axis of least overlap
    const double halfWA = entityA.getRect().w / 2.0;
    const double halfHA = entityA.getRect().h / 2.0;
    const double halfWB = entityB.getRect().w / 2.0;
    const double halfHB = entityB.getRect().h / 2.0;

    const double deltaX = (entityA.getX() + halfWA) - (entityB.getX() + halfWB);
    const double deltaY = (entityA.getY() + halfHA) - (entityB.getY() + halfHB);
    const double overlapX = halfWA + halfWB - std::abs(deltaX);
    const double overlapY = halfHA + halfHB - std::abs(deltaY);
    if (overlapX <= 0 || overlapY <= 0) return;

    if (overlapX < overlapY) {
        // Horizontal collision
        entityA.moveBy(deltaX > 0 ? overlapX : -overlapX, 0);
        entityA.setVelocityX(0);
    } else if (deltaY > 0) {
        // Hit B from below
        entityA.moveBy(0, overlapY);
        if (entityA.getVelocityY() < 0) entityA.setVelocityY(0);
    } else {
        // Landed on top of B
        entityA.moveBy(0, -overlapY);
        if (entityA.getVelocityY() > 0) entityA.setVelocityY(0);
        entityA.setGrounded(true, entityB.getEntityID());
    }
}
