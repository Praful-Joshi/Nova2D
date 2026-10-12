#ifndef TAG_ROUND_HUD_HPP
#define TAG_ROUND_HUD_HPP

#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <cstdint>
#include <string>

#include "tag_peer.hpp"

namespace hud {

// Round time as M:SS, rounded up so the timer reads 0:00 only when time is up
std::string formatRoundTime(int64_t remainingMs);

// Draws the Round's screen-space HUD: this Peer's ID and the Round, the Lobby list, the 3-2-1
// Countdown, the Round timer and who is It, the Results screen (each player's It time and the
// winner) and the Spectator notice
void drawRoundHud(SDL_Renderer* renderer, TagPeer& peer);

}  // namespace hud

#endif
