#include "render.hpp"

#include <cmath>


/**
 *   Clears the screen and sets draw color to blue
 */
void clearScreen(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderClear(renderer);
}

/**
 *   Renders entities added to the entities vector
 */
void renderEntities(SDL_Renderer* renderer, Game& game) {
    for (const auto& [entityId, entityPtr] :
         game.getEntities()) {     // Iterate over the map of entities
        if (!entityPtr) continue;  // Check for null pointers
        if (!entityPtr->isActive()) continue;  // Out of the world, e.g. waiting to respawn
        if (game.entityHasProperty(Property::DeathZone, entityId) ||
            game.entityHasProperty(Property::SpawnPoint, entityId) ||
            game.entityHasProperty(Property::SideBoundary, entityId)) {
            continue;  // Skip rendering death zones and spawn points
        }
        SDL_Rect scaledRect = entityPtr->getRect();  // Access the rectangle for the entity

        // World Coordinates to screen: draw relative to this Peer's Camera
        const Camera& camera = game.getCamera();
        scaledRect.x = static_cast<int>(std::lround(entityPtr->getX() - camera.getX()));
        scaledRect.y = static_cast<int>(std::lround(entityPtr->getY() - camera.getY()));

        // Scale from the design size to the current window size if enabled
        if (game.getIsProportionalScaling()) {
            const double sx = static_cast<double>(game.getCurrentWidth()) / game.getBaseWidth();
            const double sy = static_cast<double>(game.getCurrentHeight()) / game.getBaseHeight();
            const int left = static_cast<int>(scaledRect.x * sx);
            const int top = static_cast<int>(scaledRect.y * sy);
            scaledRect.w = static_cast<int>((scaledRect.x + scaledRect.w) * sx) - left;
            scaledRect.h = static_cast<int>((scaledRect.y + scaledRect.h) * sy) - top;
            scaledRect.x = left;
            scaledRect.y = top;
        }

        const auto& color =
            entityPtr->getColor();  // Store color in a reference to avoid multiple calls
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &scaledRect);
    }
}
/**
 *   Presents screen
 */
void presentScreen(SDL_Renderer* renderer) { SDL_RenderPresent(renderer); }
