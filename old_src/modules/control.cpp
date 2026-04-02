#include "control.hpp"

void ControlAttribute::update(Entity& entity, Game& game) {
    // Raise a control event
    if (game.getEventManager()->isRecording) {
        const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
        // Define keys of interest
        SDL_Scancode keysOfInterest[] = {SDL_SCANCODE_UP,    SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
                                         SDL_SCANCODE_RIGHT, SDL_SCANCODE_T,    SDL_SCANCODE_P,
                                         SDL_SCANCODE_1,     SDL_SCANCODE_2,    SDL_SCANCODE_3};

        // Process key presses and releases
        for (SDL_Scancode key : keysOfInterest) {
            if (currentKeyStates[key]) {
                game.keysPressed.push_back(key);
            }
        }
    }
    EventContext context = {&game, &entity};
    Event controlEvent(EventType::CONTROL, context, 3, 3);
    game.getEventManager()->raise(controlEvent);
}
