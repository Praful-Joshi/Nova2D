#include "gravity.hpp"

void GravityAttribute::update(Entity& entity, Game& game) {
    entity.setVelocityY(entity.getVelocityY() + game.getGravityStrength());
    entity.setRectY(entity.getRect().y + entity.getVelocityY());
}