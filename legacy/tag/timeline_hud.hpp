#ifndef TAG_TIMELINE_HUD_HPP
#define TAG_TIMELINE_HUD_HPP

#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <cstdint>
#include <string>

#include "tag_peer.hpp"

namespace hud {

// What a Peer is told about a paused Timeline: "PAUSED" on the Host, which paused it,
// "PAUSED BY HOST" elsewhere; empty while running
std::string pauseNotice(bool paused, bool isHost);

// The game speed for a tic in nanoseconds per Timeline unit, e.g. "2×" or "0.5×"; empty at
// normal speed
std::string speedBadge(int64_t tic);

// Draws the pause notice in the middle of the screen and the speed badge under the Round timer
void drawTimelineHud(SDL_Renderer* renderer, TagPeer& peer);

}  // namespace hud

#endif
