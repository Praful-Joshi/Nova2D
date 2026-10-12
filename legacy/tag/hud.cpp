#include "hud.hpp"

#include <cmath>

#include "arena.hpp"
#include "control.hpp"

namespace hud {

namespace {

constexpr int BAR_WIDTH = arena::PLAYER_SIZE;
constexpr int BAR_HEIGHT = 4;
constexpr int BAR_GAP = 2;     // between the player and the first bar, and between the bars
constexpr int BAR_BORDER = 1;  // dark outline so the bars show against any background

constexpr SDL_Color BACKGROUND_COLOR{30, 30, 30, 255};
constexpr SDL_Color DASH_COLOR{255, 255, 255, 255};
constexpr SDL_Color SHRINK_COLOR{255, 140, 40, 255};

void fill(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

// A bar at (x, y) in screen pixels, filled from the left in proportion to readiness (0..1)
void drawBar(SDL_Renderer* renderer, int x, int y, double readiness, SDL_Color color) {
    fill(renderer,
         SDL_Rect{x - BAR_BORDER, y - BAR_BORDER, BAR_WIDTH + 2 * BAR_BORDER,
                  BAR_HEIGHT + 2 * BAR_BORDER},
         BACKGROUND_COLOR);
    const int filled = static_cast<int>(std::lround(readiness * BAR_WIDTH));
    if (filled > 0) fill(renderer, SDL_Rect{x, y, filled, BAR_HEIGHT}, color);
}

}  // namespace

void drawAbilityBars(SDL_Renderer* renderer, Game& game, int playerEntityId, int cameraX) {
    auto player = game.getEntity(playerEntityId);
    auto control = game.getAttribute<ControlAttribute>(Property::Control, playerEntityId);
    if (!player || !control) return;

    const int64_t now = game.getTimeline()->getTime();
    const SDL_Rect& rect = player->getRect();
    // Centred under the player, whatever its size
    const int x = rect.x - cameraX + (rect.w - BAR_WIDTH) / 2;
    const int y = rect.y + rect.h + BAR_GAP + BAR_BORDER;
    drawBar(renderer, x, y, control->dashReadiness(now), DASH_COLOR);
    drawBar(renderer, x, y + BAR_HEIGHT + BAR_GAP, control->shrinkReadiness(now), SHRINK_COLOR);
}

}  // namespace hud
