#ifndef TAG_PLAYER_EFFECTS_HPP
#define TAG_PLAYER_EFFECTS_HPP

#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include "minimap.hpp"
#include "tag_peer.hpp"

namespace hud {

// Draws Tag's overlays on the players, over the rendered world: It has a red outline and a
// pulsing glow, and a player with Tag-back Immunity flashes. Follows the Camera.
void drawPlayerEffects(SDL_Renderer* renderer, TagPeer& peer);

// Minimap markers with It ringed in red
minimap::MarkerStyle tagMarkerStyle(TagPeer& peer);

}  // namespace hud

#endif
