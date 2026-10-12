#ifndef TAG_HUD_HPP
#define TAG_HUD_HPP

#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include "game.hpp"

namespace hud {

// Draws two small bars under the player showing how ready its dash (top) and shrink (bottom)
// are: empty just after use, full when ready. cameraX is the Camera's offset from World
// Coordinates, so the bars follow the player on screen. Draws nothing for an Entity without a
// ControlAttribute.
void drawAbilityBars(SDL_Renderer* renderer, Game& game, int playerEntityId, int cameraX);

}  // namespace hud

#endif
