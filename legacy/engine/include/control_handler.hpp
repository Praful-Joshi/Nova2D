#ifndef CONTROL_HANDLER
#define CONTROL_HANDLER

#include "eventmanager.hpp"
#include "game.hpp"

// Turns the held keys into running, jumping, dashing and shrinking. One handler serves every
// controlled Entity, so it keeps no state: each Entity's input state lives on its ControlAttribute.
class ControlHandler : public EventHandler {
public:
    ControlHandler() = default;
    ~ControlHandler() override = default;
    void onEvent(const Event& event) override;

    // Two presses of the same direction key at most this far apart (Timeline units) dash
    static constexpr int64_t DOUBLE_TAP_WINDOW = 250;
    // How far a dash teleports, in pixels
    static constexpr int DASH_DISTANCE = 200;

private:
    static void updateGameState(Entity& entity, Game& game);
};

#endif
