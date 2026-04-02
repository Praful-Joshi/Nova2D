#ifndef DEATH_HANDLER
#define DEATH_HANDLER

#include "eventmanager.hpp"
#include "game.hpp"

class DeathHandler : public EventHandler {
public:
    DeathHandler() = default;
    ~DeathHandler() override = default;
    void onEvent(const Event& event) override;

private:
    void updateGameState(Entity& entity, Game& game);
};

#endif