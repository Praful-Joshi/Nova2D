#include "minimap.hpp"

#include <algorithm>
#include <cmath>

#include "arena.hpp"

namespace minimap {

namespace {

constexpr SDL_Color BACKGROUND{15, 15, 60, 255};
constexpr SDL_Color BORDER{120, 120, 160, 255};
constexpr SDL_Color VIEW{255, 255, 255, 255};

// Terrain is drawn as its own colour faded halfway to the background, so players stand out
SDL_Color muted(SDL_Color color) {
    return SDL_Color{static_cast<Uint8>((color.r + BACKGROUND.r) / 2),
                     static_cast<Uint8>((color.g + BACKGROUND.g) / 2),
                     static_cast<Uint8>((color.b + BACKGROUND.b) / 2), 255};
}

void setColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

// A World Coordinates rect scaled into the strip whose top-left is (left, top); at least 2 px
SDL_Rect toStrip(double x, double y, int w, int h, int left, int top) {
    return SDL_Rect{left + static_cast<int>(std::lround(x / SCALE)),
                    top + static_cast<int>(std::lround(y / SCALE)),
                    std::max(2, static_cast<int>(std::lround(static_cast<double>(w) / SCALE))),
                    std::max(2, static_cast<int>(std::lround(static_cast<double>(h) / SCALE)))};
}

bool isPlayer(int entityId) {
    for (int peerId = 1; peerId <= arena::MAX_PLAYERS; ++peerId) {
        if (entityId == arena::playerEntityId(peerId)) return true;
    }
    return false;
}

}  // namespace

void drawPlayerMarker(SDL_Renderer* renderer, const Entity& player, const SDL_Rect& marker) {
    setColor(renderer, player.getColor());
    SDL_RenderFillRect(renderer, &marker);
}

void draw(SDL_Renderer* renderer, Game& game, const MarkerStyle& drawMarker) {
    const SDL_Rect strip{(game.getCurrentWidth() - arena::WIDTH / SCALE) / 2, TOP,
                         arena::WIDTH / SCALE, arena::HEIGHT / SCALE};

    setColor(renderer, BACKGROUND);
    SDL_RenderFillRect(renderer, &strip);
    // Players that leave the arena stay inside the strip
    SDL_RenderSetClipRect(renderer, &strip);

    // Terrain: everything drawn in the world that isn't a player
    for (const auto& [id, entity] : game.getEntities()) {
        if (isPlayer(id) || game.entityHasProperty(Property::DeathZone, id) ||
            game.entityHasProperty(Property::SpawnPoint, id) ||
            game.entityHasProperty(Property::SideBoundary, id)) {
            continue;
        }
        const SDL_Rect rect = toStrip(entity->getX(), entity->getY(), entity->getRect().w,
                                      entity->getRect().h, strip.x, strip.y);
        setColor(renderer, muted(entity->getColor()));
        SDL_RenderFillRect(renderer, &rect);
    }

    // The Camera's view
    const Camera& camera = game.getCamera();
    const SDL_Rect view = toStrip(camera.getX(), camera.getY(), camera.getViewWidth(),
                                  camera.getViewHeight(), strip.x, strip.y);
    setColor(renderer, VIEW);
    SDL_RenderDrawRect(renderer, &view);

    // Players on top, in Peer ID order
    for (int peerId = 1; peerId <= arena::MAX_PLAYERS; ++peerId) {
        auto player = game.getEntity(arena::playerEntityId(peerId));
        if (!player) continue;
        const SDL_Rect marker = toStrip(player->getX(), player->getY(), player->getRect().w,
                                        player->getRect().h, strip.x, strip.y);
        drawMarker(renderer, *player, marker);
    }

    SDL_RenderSetClipRect(renderer, nullptr);
    setColor(renderer, BORDER);
    const SDL_Rect border{strip.x - 1, strip.y - 1, strip.w + 2, strip.h + 2};
    SDL_RenderDrawRect(renderer, &border);
}

}  // namespace minimap
