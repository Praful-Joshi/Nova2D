#include "collision_handler.hpp"

void CollisionHandler::onEvent(const Event& event) {
    if (event.type == EventType::COLLISION) {
        updateGameState(*event.context.entity, *event.context.game);
    }
}

void CollisionHandler::updateGameState(Entity& entity, Game& game) {
    // Iterate through all entities in the game
    auto gameEntities = game.getEntities();
    for (const auto& [otherEntityId, otherEntityPtr] : gameEntities) {
        if (entity.getEntityID() == otherEntityId) continue;  // Skip self-collision

        // Check for intersection between entities
        if (SDL_HasIntersection(&entity.getRect(), &otherEntityPtr->getRect())) {
            resolveCollision(entity, *otherEntityPtr);
        }
    }
}

void CollisionHandler::resolveCollision(Entity& entityA, Entity& entityB) {
    // Collision resolution logic here
    // Example: Adjust positions based on collision response
    SDL_Rect& rectA = entityA.getRect();
    SDL_Rect& rectB = entityB.getRect();

    // Calculate the delta and overlap in both x and y axes
    int deltaX = (rectA.x + rectA.w / 2) - (rectB.x + rectB.w / 2);
    int deltaY = (rectA.y + rectA.h / 2) - (rectB.y + rectB.h / 2);
    int overlapX = (rectA.w + rectB.w) / 2 - abs(deltaX);
    int overlapY = (rectA.h + rectB.h) / 2 - abs(deltaY);

    // Resolve the collision based on whether entities are movable
    bool aMovable = entityA.getisMovable();
    bool bMovable = entityB.getisMovable();

    if (overlapX < overlapY) {
        // Horizontal collision
        if (deltaX > 0) {
            if (aMovable) rectA.x += overlapX;  // Move A to the right
            if (bMovable) rectB.x -= overlapX;  // Move B to the left
        } else {
            if (aMovable) rectA.x -= overlapX;  // Move A to the left
            if (bMovable) rectB.x += overlapX;  // Move B to the right
        }
        if (aMovable) entityA.setVelocityX(0);  // Stop horizontal velocity for A
        if (bMovable) entityB.setVelocityX(0);  // Stop horizontal velocity for B
    } else {
        // Vertical collision
        if (deltaY > 0) {
            if (aMovable) rectA.y += overlapY;  // Move A down
            if (bMovable) rectB.y -= overlapY;  // Move B up
        } else {
            if (aMovable) rectA.y -= overlapY;  // Move A up
            if (bMovable) rectB.y += overlapY;  // Move B down
        }
        if (aMovable) entityA.setVelocityY(0);  // Stop vertical velocity for A
        if (bMovable) entityB.setVelocityY(0);  // Stop vertical velocity for B
    }
    // Optionally: Stop the movement for each entity, adjust velocities, etc.
}