#include "death_handler.hpp"

void DeathHandler::onEvent(const Event& event) {
    if (event.type == EventType::DEATH) {
        updateGameState(*event.context.entity, *event.context.game);
    }
}

void DeathHandler::updateGameState(Entity& entity, Game& game) {
    // The event's Entity is the death zone
    auto deathZone =
        game.getAttribute<DeathZoneAttribute>(Property::DeathZone, entity.getEntityID());
    if (!deathZone) return;

    const int64_t now = game.getTimeline()->getTime();
    auto& pending = deathZone->getPendingRespawns();

    // Players this Peer controls who just touched the zone leave the world until they respawn
    for (const auto& [id, otherEntity] : game.getEntities()) {
        if (id == entity.getEntityID() || !otherEntity->isActive()) continue;
        if (!game.entityHasProperty(Property::Control, id)) continue;
        if (!SDL_HasIntersection(&entity.getRect(), &otherEntity->getRect())) continue;

        otherEntity->setActive(false);
        otherEntity->setVelocityX(0);
        otherEntity->setVelocityY(0);
        otherEntity->setGrounded(false);
        pending[id] = now + deathZone->getRespawnDelay();
    }

    // Players whose delay is over come back
    for (auto it = pending.begin(); it != pending.end();) {
        if (now < it->second) {
            ++it;
            continue;
        }
        auto player = game.getEntity(it->first);
        if (player) respawn(*player, *deathZone, game);
        it = pending.erase(it);
    }
}

void DeathHandler::respawn(Entity& player, const DeathZoneAttribute& deathZone, Game& game) {
    if (deathZone.getResetsWorld()) {
        for (const auto& [id, position] : deathZone.getOriginalPositions()) {
            auto entity = game.getEntity(id);
            if (!entity) continue;
            entity->setPosition(position.x, position.y);
            if (game.entityHasProperty(Property::Control, id)) {
                entity->setVelocityX(0);
                entity->setVelocityY(0);
            }
        }
    }

    // The player's own SpawnPoint, or the zone's if it has none
    auto spawn = game.getEntity(player.getSpawnPointId());
    if (!spawn) spawn = game.getEntity(deathZone.getSpawnPointId());
    if (spawn) player.setPosition(spawn->getX(), spawn->getY());

    player.setVelocityX(0);
    player.setVelocityY(0);
    player.setActive(true);
}
