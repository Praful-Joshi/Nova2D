#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include "entity.hpp"
#include "game.hpp"

void clearScreen(SDL_Renderer* renderer);

void renderEntities(SDL_Renderer* renderer, Game& game);

void presentScreen(SDL_Renderer* renderer);
