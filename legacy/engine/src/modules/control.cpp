#include "control.hpp"

void ControlAttribute::update(Entity& entity, Game& game) {
    // Raise a control event
    EventContext context = {&game, &entity};
    Event controlEvent(EventType::CONTROL, context, 3, 3);
    game.getEventManager()->raise(controlEvent);
}
