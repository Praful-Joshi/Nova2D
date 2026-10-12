#ifndef TAG_ARENA_HPP
#define TAG_ARENA_HPP

#include "game.hpp"

// Tag's fixed, hand-made arena, identical on every Peer. All positions are World Coordinates.
namespace arena {

constexpr int WIDTH = 2560;
constexpr int HEIGHT = 720;
constexpr int GROUND_TOP = 660;
constexpr int PLAYER_SIZE = 40;

// Entity ID of the player owned by the given Peer
constexpr int playerEntityId(int peerId) { return 1000 + peerId; }

// Fixed colour for each Peer ID
SDL_Color playerColor(int peerId);

// Adds the arena's Entities and their Attributes to the game
void build(Game& game);

// Adds the player Entity for the given Peer; controllable only for the local Peer
std::shared_ptr<Entity> addPlayer(Game& game, int peerId, bool isLocal);

}  // namespace arena

#endif
