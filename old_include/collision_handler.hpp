#ifndef COLLISION_HANDLER
#define COLLISION_HANDLER

#include "eventmanager.hpp"
#include "game.hpp"

class CollisionHandler : public EventHandler {
public:
    CollisionHandler() = default;
    ~CollisionHandler() override = default;
    void onEvent(const Event& event) override;

private:
    void updateGameState(Entity& entity, Game& game);
    void resolveCollision(Entity& entityA, Entity& entityB);
};

#endif