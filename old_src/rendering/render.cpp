#include "render.hpp"

#include <mutex>

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
    std::mutex game_entities_mutex;
    game_entities_mutex.lock();
    for (const auto& [entityId, entityPtr] :
         game.getEntities()) {     // Iterate over the map of entities
        if (!entityPtr) continue;  // Check for null pointers
        if (game.entityHasProperty(Property::DeathZone, entityId) ||
            game.entityHasProperty(Property::SpawnPoint, entityId) ||
            game.entityHasProperty(Property::SideBoundary, entityId)) {
            continue;  // Skip rendering death zones and spawn points
        }
        SDL_Rect scaledRect = entityPtr->getRect();  // Access the rectangle for the entity

        // Scale the rectangle size proportionally based on the current window size if enabled
        if (game.getIsProportionalScaling()) {
            scaledRect.w = static_cast<int>(scaledRect.w * game.getCurrentWidth() / 1920.0);
            scaledRect.h = static_cast<int>(scaledRect.h * game.getCurrentHeight() / 1080.0);
            scaledRect.x = static_cast<int>(scaledRect.x * game.getCurrentWidth() / 1920.0);
            scaledRect.y = static_cast<int>(scaledRect.y * game.getCurrentHeight() / 1080.0);
        }

        const auto& color =
            entityPtr->getColor();  // Store color in a reference to avoid multiple calls
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &scaledRect);
    }
    game_entities_mutex.unlock();
}
/**
 *   Presents screen
 */
void presentScreen(SDL_Renderer* renderer) { SDL_RenderPresent(renderer); }
