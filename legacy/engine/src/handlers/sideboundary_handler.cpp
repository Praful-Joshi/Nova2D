#include "sideboundary_handler.hpp"

void SideBoundaryHandler::onEvent(const Event& event) {
    if (event.type == EventType::SIDEBOUNDARY) {
        auto it = event.parameters.find("ISLEFTBOUNDARY");
        if (it != event.parameters.end() && it->second.type == Variant::Type::BOOL) {
            isLeftBoundary = it->second.asBool;
        }
        updateGameState(*event.context.entity, *event.context.game);
    }
}

void SideBoundaryHandler::updateGameState(Entity& zone, Game& game) {
    if (!game.entityHasProperty(Property::SideBoundary, zone.getEntityID())) return;

    Camera& camera = game.getCamera();
    const SDL_Rect& zoneRect = zone.getRect();

    for (const auto& [id, entity] : game.getEntities()) {
        // Only the local player scrolls this Peer's Camera
        if (!game.entityHasProperty(Property::Control, id)) continue;

        // The player's left and right edges on screen
        const double left = entity->getX() - camera.getX();
        const double right = left + entity->getRect().w;

        // Scroll just enough to keep the player at the zone's inner edge, so the view follows
        // the player smoothly at the player's own speed
        if (isLeftBoundary) {
            const double innerEdge = zoneRect.x + zoneRect.w;
            if (left < innerEdge) camera.scrollBy(left - innerEdge, 0);
        } else {
            const double innerEdge = zoneRect.x;
            if (right > innerEdge) camera.scrollBy(right - innerEdge, 0);
        }
    }
}
