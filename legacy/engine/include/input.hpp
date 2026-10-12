#ifndef INPUT_HPP
#define INPUT_HPP

#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <array>
#include <initializer_list>

// The keys held during one tick. The game fills it from the keyboard; tests build it directly.
class Keys {
public:
    Keys() = default;
    Keys(std::initializer_list<SDL_Scancode> held);

    // Snapshot of the real keyboard (requires SDL to be initialised)
    static Keys fromKeyboard();

    [[nodiscard]] bool held(SDL_Scancode key) const { return down[key]; }
    void set(SDL_Scancode key, bool isHeld) { down[key] = isHeld; }

private:
    std::array<bool, SDL_NUM_SCANCODES> down{};
};

#endif
