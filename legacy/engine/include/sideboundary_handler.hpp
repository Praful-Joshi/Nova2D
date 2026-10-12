#ifndef SIDEBOUNDARY_HANLDER
#define SIDEBOUNDARY_HANLDER

#include "eventmanager.hpp"
#include "game.hpp"

// Scrolls the Camera when the local (controllable) player enters a SideBoundary zone. The zone's
// rect is in screen space, so it is compared against the player's screen position (World
// Coordinates minus the Camera). Entities never move.
class SideBoundaryHandler : public EventHandler {
public:
    SideBoundaryHandler() = default;
    ~SideBoundaryHandler() override = default;
    void onEvent(const Event& event) override;

private:
    void updateGameState(Entity& zone, Game& game);
    bool isLeftBoundary = false;
};

#endif