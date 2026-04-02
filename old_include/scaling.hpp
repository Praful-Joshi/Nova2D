#ifndef SCALING_HPP
#define SCALING_HPP

#include "entity.hpp"

enum class ScalingMode { ConstantSize, Proportional };

void updateWindowSize(SDL_Window* window, int cur_w, int cur_h);

#endif
void updateWindowSize(SDL_Window* window, int cur_w, int cur_h);
