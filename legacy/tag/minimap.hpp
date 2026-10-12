#ifndef TAG_MINIMAP_HPP
#define TAG_MINIMAP_HPP

#include <functional>

#include "entity.hpp"
#include "game.hpp"

// Tag's minimap: a strip across the top of the screen showing the whole arena, every player's
// position in their colour, and the Camera's view. Drawn in screen space, like the rest of the HUD.
namespace minimap {

// Arena pixels per minimap pixel
constexpr int SCALE = 6;
// Gap between the top of the screen and the strip
constexpr int TOP = 8;

// Draws one player on the minimap; marker is the player's rect scaled into the strip
using MarkerStyle =
    std::function<void(SDL_Renderer* renderer, const Entity& player, const SDL_Rect& marker)>;

// The default marker: a box filled with the player's colour
void drawPlayerMarker(SDL_Renderer* renderer, const Entity& player, const SDL_Rect& marker);

// Draws the minimap, using drawMarker for each player (to highlight It, for example)
void draw(SDL_Renderer* renderer, Game& game, const MarkerStyle& drawMarker = drawPlayerMarker);

}  // namespace minimap

#endif
