#include "sideboundary.hpp"

#include <iostream>

void SideBoundaryAttribute::update(Entity& entity, Game& game) {
    EventContext context = {&game, &entity};
    Event sideboundaryEvent(EventType::SIDEBOUNDARY, context, 0, 0);
    Variant isleft;
    isleft.type = Variant::Type::BOOL;
    isleft.asBool = isLeftBoundary;
    sideboundaryEvent.parameters["ISLEFTBOUNDARY"] = isleft;
    game.getEventManager()->raise(sideboundaryEvent);
}
