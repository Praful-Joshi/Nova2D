#include "player_effects.hpp"

#include <cmath>

#include "arena.hpp"

namespace hud {

namespace {

constexpr SDL_Color IT_RED{230, 30, 30, 255};
constexpr int OUTLINE_WIDTH = 3;
constexpr int GLOW_WIDTH = 10;       // widest glow, beyond the outline
constexpr Uint32 GLOW_PERIOD_MS = 900;
constexpr Uint32 FLASH_PERIOD_MS = 250;  // one flash on, one off
constexpr double TAU = 6.283185307179586;

// Draws `width` rect outlines, each one pixel further out than the last
void drawRing(SDL_Renderer* renderer, const SDL_Rect& rect, int inset, int width) {
    for (int i = inset; i < inset + width; ++i) {
        const SDL_Rect ring{rect.x - i - 1, rect.y - i - 1, rect.w + 2 * (i + 1),
                            rect.h + 2 * (i + 1)};
        SDL_RenderDrawRect(renderer, &ring);
    }
}

// A red glow fading outwards, growing and shrinking over time
void drawGlow(SDL_Renderer* renderer, const SDL_Rect& rect, Uint32 nowMs) {
    const double phase = static_cast<double>(nowMs % GLOW_PERIOD_MS) / GLOW_PERIOD_MS;
    const double pulse = 0.5 + 0.5 * std::sin(phase * TAU);  // 0..1
    const int width = 3 + static_cast<int>(std::lround(pulse * (GLOW_WIDTH - 3)));
    for (int i = 0; i < width; ++i) {
        const auto alpha = static_cast<Uint8>(190 * (width - i) / width);
        SDL_SetRenderDrawColor(renderer, IT_RED.r, IT_RED.g, IT_RED.b, alpha);
        drawRing(renderer, rect, OUTLINE_WIDTH + i, 1);
    }
}

// Where the renderer draws the player on screen: relative to the Camera, scaled to the window
// if proportional scaling is on
SDL_Rect toScreen(Game& game, const Entity& player) {
    const Camera& camera = game.getCamera();
    SDL_Rect rect{static_cast<int>(std::lround(player.getX() - camera.getX())),
                  static_cast<int>(std::lround(player.getY() - camera.getY())),
                  player.getRect().w, player.getRect().h};
    if (game.getIsProportionalScaling()) {
        const double sx = static_cast<double>(game.getCurrentWidth()) / game.getBaseWidth();
        const double sy = static_cast<double>(game.getCurrentHeight()) / game.getBaseHeight();
        const int left = static_cast<int>(rect.x * sx);
        const int top = static_cast<int>(rect.y * sy);
        rect.w = static_cast<int>((rect.x + rect.w) * sx) - left;
        rect.h = static_cast<int>((rect.y + rect.h) * sy) - top;
        rect.x = left;
        rect.y = top;
    }
    return rect;
}

}  // namespace

void drawPlayerEffects(SDL_Renderer* renderer, TagPeer& peer) {
    Game& game = peer.game();
    const Uint32 nowMs = SDL_GetTicks();  // real time, so effects keep moving while paused
    const int it = peer.itPeerId();

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int peerId = 1; peerId <= arena::MAX_PLAYERS; ++peerId) {
        auto player = game.getEntity(arena::playerEntityId(peerId));
        if (!player || !player->isActive()) continue;
        const SDL_Rect rect = toScreen(game, *player);

        if (peerId == it) {
            drawGlow(renderer, rect, nowMs);
            SDL_SetRenderDrawColor(renderer, IT_RED.r, IT_RED.g, IT_RED.b, IT_RED.a);
            drawRing(renderer, rect, 0, OUTLINE_WIDTH);
        } else if (peer.isImmune(peerId) && (nowMs / FLASH_PERIOD_MS) % 2 == 0) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 170);
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

minimap::MarkerStyle tagMarkerStyle(TagPeer& peer) {
    return [&peer](SDL_Renderer* renderer, const Entity& player, const SDL_Rect& marker) {
        if (player.getEntityID() == arena::playerEntityId(peer.itPeerId())) {
            const SDL_Rect ring{marker.x - 2, marker.y - 2, marker.w + 4, marker.h + 4};
            SDL_SetRenderDrawColor(renderer, IT_RED.r, IT_RED.g, IT_RED.b, IT_RED.a);
            SDL_RenderFillRect(renderer, &ring);
        }
        minimap::drawPlayerMarker(renderer, player, marker);
    };
}

}  // namespace hud
