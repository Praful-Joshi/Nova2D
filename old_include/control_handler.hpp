#ifndef CONTROL_HANDLER
#define CONTROL_HANDLER

#include "eventmanager.hpp"
#include "game.hpp"

class ControlHandler : public EventHandler {
public:
    ControlHandler() = default;
    ~ControlHandler() override = default;
    void onEvent(const Event& event) override;

private:
    void updateGameState(Entity& entity, Game& game);

    // New member variables
    Uint8 previousKeyStates[SDL_NUM_SCANCODES];  // To store previous key states
    std::deque<std::pair<SDL_Scancode, Uint32>>
        keySequence;  // To store key presses with timestamps

    // Variables for the shrink mechanic
    bool isShrunk = false;
    Uint32 shrinkStartTime = 0;

    // Helper functions for chord and sequence detection
    void checkForChords(const Uint8* currentKeyStates, Entity& entity, Game& game,
                        Uint32 timestamp);
    void checkForSequences(SDL_Scancode key, Uint32 timestamp, Entity& entity, Game& game);
};

#endif
