#ifndef DEATH_ZONE_ATTRIBUTE_HPP
#define DEATH_ZONE_ATTRIBUTE_HPP

#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>

#include "attribute.hpp"
#include "eventmanager.hpp"
#include "game.hpp"

// A zone that takes a player who touches it out of the world and respawns them later.
//
// Only Entities with the Control property (the players this Peer controls) are affected. The
// player is made inactive (hidden, unable to act) with its velocity zeroed; respawnDelay
// Timeline units later it reappears, at rest, at its own SpawnPoint, or at this zone's SpawnPoint
// if it has none. Nothing else in the world changes unless resetWorld is set: then, as the player
// respawns, every Entity in originalPositions also returns to its position there (for
// single-player games that restart the level).
class DeathZoneAttribute : public Attribute {
public:
    explicit DeathZoneAttribute(int spawnId, int64_t respawnDelay = 0, bool resetWorld = false,
                                std::unordered_map<int, SDL_Rect> originalPositions = {})
        : spawnPointId(spawnId),
          respawnDelay(respawnDelay),
          resetWorld(resetWorld),
          originalPositions(std::move(originalPositions)) {}
    void update(Entity& entity, Game& game) override;

    // SpawnPoint used for players that don't have their own
    [[nodiscard]] int getSpawnPointId() const { return spawnPointId; }
    [[nodiscard]] int64_t getRespawnDelay() const { return respawnDelay; }
    [[nodiscard]] bool getResetsWorld() const { return resetWorld; }
    [[nodiscard]] const std::unordered_map<int, SDL_Rect>& getOriginalPositions() const {
        return originalPositions;
    }

    // Players waiting to respawn: Entity ID -> Timeline time at which they respawn
    std::map<int, int64_t>& getPendingRespawns() { return pendingRespawns; }

private:
    int spawnPointId;
    int64_t respawnDelay;
    bool resetWorld;
    std::unordered_map<int, SDL_Rect> originalPositions;
    std::map<int, int64_t> pendingRespawns;
};

#endif  // DEATH_ZONE_ATTRIBUTE_HPP
