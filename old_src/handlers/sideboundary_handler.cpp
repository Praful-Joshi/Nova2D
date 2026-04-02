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

void SideBoundaryHandler::updateGameState(Entity& entity, Game& game) {
    int scrollAmount = isLeftBoundary ? 10 : -10;

    auto entities = game.getEntities();

    for (const auto& [id, otherEntity] : entities) {
        // Skip checking itself
        if (entity.getEntityID() == id) {
            continue;
        }

        // Check if this entity is a death zone
        if (!game.entityHasProperty(Property::SideBoundary, entity.getEntityID())) {
            continue;
        }

        // Check for collision
        if (SDL_HasIntersection(&entity.getRect(), &otherEntity->getRect())) {
            // Check if the other entity has the Control property (only teleport control entities)
            if (!game.entityHasProperty(Property::Control, otherEntity->getEntityID())) {
                continue;
            }
            // Scroll all entities horizontally by scrollAmount
            for (auto& [entityId, gameEntity] : entities) {
                if (gameEntity->getEntityID() != entity.getEntityID() &&
                    !game.entityHasProperty(Property::SideBoundary, gameEntity->getEntityID())) {
                    gameEntity->setRectX(gameEntity->getRect().x + scrollAmount);
                }
            }

            return;
        }
    }
}
