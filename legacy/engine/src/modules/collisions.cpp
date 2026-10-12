#include "collisions.hpp"

void CollisionAttribute::update(Entity& entity, Game& game) {
    // Raise a collision event
    EventContext context = {&game, &entity};
    Event collisionEvent(EventType::COLLISION, context, 2, 2);
    game.getEventManager()->raise(collisionEvent);
}