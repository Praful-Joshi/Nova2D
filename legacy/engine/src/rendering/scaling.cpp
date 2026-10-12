#include "scaling.hpp"

/**
 *   Updates windows size to current width and height
 */
void updateWindowSize(SDL_Window* window, int cur_w, int cur_h) {
    SDL_GetWindowSize(window, &cur_w, &cur_h);
}