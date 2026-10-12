#include "input.hpp"

Keys::Keys(std::initializer_list<SDL_Scancode> held) {
    for (SDL_Scancode key : held) down[key] = true;
}

Keys Keys::fromKeyboard() {
    Keys keys;
    int count = 0;
    const Uint8* state = SDL_GetKeyboardState(&count);
    for (int i = 0; i < count && i < SDL_NUM_SCANCODES; ++i) keys.down[i] = state[i] != 0;
    return keys;
}
