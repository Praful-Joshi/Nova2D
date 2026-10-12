#ifndef TAG_ARENA_HPP
#define TAG_ARENA_HPP

#include <memory>

#include "game.hpp"

// Tag's fixed, hand-made arena, identical on every Peer. All positions are World Coordinates.
//
//   x: 0        800  1000       1460 1560  1760                2560
//      |  G1     | pit |   G2      |gap|   | pit |      G3         |
//
// Two horizontal platforms bridge the pits, an elevator in G2 lifts players to the central
// high ledge, and a wall in G2 leaves a low gap at ground level that only a shrunk player fits.
namespace arena {

constexpr int WIDTH = 2560;
constexpr int HEIGHT = 720;
constexpr int GROUND_TOP = 660;
constexpr int PLAYER_SIZE = 40;
constexpr int MAX_PLAYERS = 4;

// The two pits between the three ground segments
constexpr int PIT1_LEFT = 800;
constexpr int PIT1_RIGHT = 1000;
constexpr int PIT2_LEFT = 1560;
constexpr int PIT2_RIGHT = 1760;

// A low static ledge (used as a reference landing spot)
constexpr int LEDGE_X = 250;
constexpr int LEDGE_TOP = 520;

// Central high ledge, reached by the elevator
constexpr int HIGH_LEDGE_X = 1180;
constexpr int HIGH_LEDGE_TOP = 240;

// Elevator travel (top edge of the elevator)
constexpr int ELEVATOR_X = 1060;
constexpr int ELEVATOR_TOP_MIN = 240;
constexpr int ELEVATOR_TOP_MAX = 620;

// Wall with a low gap beneath it: the gap is GAP_HEIGHT tall, between the wall and the ground
constexpr int GAP_WALL_X = 1460;
constexpr int GAP_WALL_WIDTH = 40;
constexpr int GAP_HEIGHT = 30;

// Width of the SideBoundary zones at the left and right edges of the screen (screen space):
// the Camera scrolls to keep the local player at least this far from either edge
constexpr int SIDE_ZONE_WIDTH = 300;

// Entity IDs of the arena's moving platforms
constexpr int PLATFORM1_ID = 10;
constexpr int PLATFORM2_ID = 11;
constexpr int ELEVATOR_ID = 12;

// Entity IDs of the DeathZones in the two pits, and how long, in Timeline units, a player who
// falls in stays out of the world before respawning at their SpawnPoint
constexpr int PIT1_DEATHZONE_ID = 50;
constexpr int PIT2_DEATHZONE_ID = 51;
constexpr int LEFT_SIDE_ZONE_ID = 60;
constexpr int RIGHT_SIDE_ZONE_ID = 61;
constexpr int64_t RESPAWN_DELAY = 1000;

// Entity ID of the player owned by the given Peer
constexpr int playerEntityId(int peerId) { return 1000 + peerId; }

// Entity ID of the SpawnPoint for the given Peer
constexpr int spawnPointId(int peerId) { return 40 + (peerId - 1) % MAX_PLAYERS; }

// Fixed colour for each Peer ID
SDL_Color playerColor(int peerId);

// Adds the arena's Entities and their Attributes to the game
void build(Game& game);

// Adds the player Entity for the given Peer at its SpawnPoint; controllable only when local
std::shared_ptr<Entity> addPlayer(Game& game, int peerId, bool isLocal);

}  // namespace arena

#endif
