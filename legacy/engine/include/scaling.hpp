#ifndef SCALING_HPP
#define SCALING_HPP

#include "entity.hpp"

class Game;

// Copies the window's current size into the game, for proportional scaling
void updateWindowSize(SDL_Window* window, Game& game);

#endif
