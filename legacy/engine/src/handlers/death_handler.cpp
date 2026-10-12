#include "death_handler.hpp"

void DeathHandler::onEvent(const Event& event) {
    if (event.type == EventType::DEATH) {
        updateGameState(*event.context.entity, *event.context.game);
    }
}

void DeathHandler::updateGameState(Entity& entity, Game& game) {
    auto entities = game.getEntities();
    auto spawnPointId = entity.getSpawnPointId();
    auto originalPositions = game.getOriginalPositions();

    for (const auto& [id, otherEntity] : entities) {
        // Skip itself
        if (entity.getEntityID() == id) {
            continue;
        }

        // Confirm this entity is a death zone
        if (!game.entityHasProperty(Property::DeathZone, entity.getEntityID())) {
            continue;
        }
        // Check for collision with control entity
        if (SDL_HasIntersection(&entity.getRect(), &otherEntity->getRect())) {
            if (!game.entityHasProperty(Property::Control, otherEntity->getEntityID())) {
                continue;
            }

            // Find the specified spawn point and teleport the control entity
            auto spawnEntity = game.getEntity(spawnPointId);
            if (spawnEntity) {
                // std::cout << "Teleporting control entity to spawn point and resetting all
                // entities to original positions." << std::endl;

                // Reset the control entity to the spawn point
                otherEntity->setRectX(spawnEntity->getRect().x);
                otherEntity->setRectY(spawnEntity->getRect().y);
                otherEntity->setVelocityX(0);
                otherEntity->setVelocityY(0);

                // Reset all entities (including control, death zone, and others) to their original
                // positions
                for (auto& [entityId, gameEntity] : entities) {
                    if (originalPositions.find(entityId) != originalPositions.end()) {
                        SDL_Rect originalPos = originalPositions[entityId];
                        gameEntity->setRectX(originalPos.x);
                        gameEntity->setRectY(originalPos.y);
                        // std::cout << "Entity " << entityId << " reset to (" << originalPos.x <<
                        // ", " << originalPos.y << ")" << std::endl;
                    }
                }

                return;  // Exit after resetting all entities
            } else {
                // std::cerr << "Spawn point with ID " << spawnPointId << " not found!" <<
                // std::endl;
            }
        }
    }
}