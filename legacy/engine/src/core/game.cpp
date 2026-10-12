#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "eventmanager.hpp"
#include "platform.hpp"

void Game::addEntity(const std::shared_ptr<Entity>& entity) {
    // A new entity's time step starts now
    if (timeline != nullptr) entity->startClock(timeline->getTime());
    entities[entity->getEntityID()] = entity;
}

void Game::removeEntity(const std::shared_ptr<Entity>& entity) {
    entities.erase(entity->getEntityID());
    for (auto& [property, attributes] : properties) attributes.erase(entity->getEntityID());
}

// Property management
void Game::addProperty(Property propertyType, int entityId, std::shared_ptr<Attribute> attribute) {
    properties[propertyType][entityId] = std::move(attribute);
}

void Game::updateProperties(Property propertyType, Timeline* timeline) {
    auto it = properties.find(propertyType);
    if (it != properties.end()) {
        for (const auto& [entityId, attribute] : it->second) {
            auto entity = getEntity(entityId);
            if (entity != nullptr && entity->isActive() && attribute) {
                // Use the updated method signature
                attribute->update(*entity, *this);
            }
        }
    }
}

void Game::update() {
    // Each entity's own time step since its last frame
    const int64_t now = timeline->getTime();
    int64_t longestFrame = 0;
    for (const auto& [id, entity] : entities) {
        const int64_t frame = std::min(entity->beginFrame(now), MAX_FRAME);
        entity->setDeltaTime(static_cast<double>(frame));
        longestFrame = std::max(longestFrame, frame);
    }

    updateProperties(Property::Control, timeline);
    eventManager->processEvents();

    // Physics in equal substeps so fast movement can't pass through thin solids
    const int steps = std::max(1, static_cast<int>(std::ceil(longestFrame / MAX_STEP)));
    for (int step = 0; step < steps; ++step) {
        for (const auto& [id, entity] : entities) {
            const double frame = static_cast<double>(std::min(entity->getFrameDelta(), MAX_FRAME));
            entity->setDeltaTime(frame / steps);
        }
        updateProperties(Property::Move, timeline);
        carryRiders();
        updateProperties(Property::Gravity, timeline);
        updateProperties(Property::Collide, timeline);
        eventManager->processEvents();
    }

    updateProperties(Property::DeathZone, timeline);
    updateProperties(Property::SideBoundary, timeline);
    eventManager->processEvents();
}

void Game::carryRiders() {
    for (const auto& [id, entity] : entities) {
        if (!entity->isActive() || !entity->isGrounded()) continue;
        auto platform = std::dynamic_pointer_cast<Platform>(getEntity(entity->getGroundEntityId()));
        if (platform) entity->moveBy(platform->getLastDeltaX(), platform->getLastDeltaY());
    }
}

std::shared_ptr<Entity> Game::getEntity(const int& GUID) {
    auto it = entities.find(GUID);  // Find the entity with the specified GUID
    if (it != entities.end()) {
        return it->second;  // Return the shared_ptr to the entity
    }
    return nullptr;  // Return nullptr if the entity is not found
}

bool Game::entityHasProperty(Property propertyType, int entityId) const {
    auto propertyIt = properties.find(propertyType);
    if (propertyIt != properties.end()) {
        const auto& entityMap = propertyIt->second;
        return entityMap.find(entityId) != entityMap.end();
    }
    return false;  // Property or entity ID not found
}
