#include "scaling.hpp"

#include "game.hpp"

void updateWindowSize(SDL_Window* window, Game& game) {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);
    game.setCurrentWidth(width);
    game.setCurrentHeight(height);
}
