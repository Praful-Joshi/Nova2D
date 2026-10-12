#pragma once
#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <iostream>

// Initialize SDL rendering window
bool init(SDL_Window*& window, SDL_Renderer*& renderer);
