#include "deathzone.hpp"

void DeathZoneAttribute::update(Entity& entity, Game& game) {
    // Raise a death event
    EventContext context = {&game, &entity};
    Event deathEvent(EventType::DEATH, context, 1, 1);
    game.getEventManager()->raise(deathEvent);
}
