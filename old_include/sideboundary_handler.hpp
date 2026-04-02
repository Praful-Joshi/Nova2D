#ifndef SIDEBOUNDARY_HANLDER
#define SIDEBOUNDARY_HANLDER

#include "eventmanager.hpp"
#include "game.hpp"

class SideBoundaryHandler : public EventHandler {
public:
    SideBoundaryHandler() = default;
    ~SideBoundaryHandler() override = default;
    void onEvent(const Event& event) override;

private:
    void updateGameState(Entity& entity, Game& game);
    bool isLeftBoundary;
};

#endif