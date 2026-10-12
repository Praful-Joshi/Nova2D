#include "arena.hpp"

#include "collisions.hpp"
#include "control.hpp"
#include "gravity.hpp"

namespace arena {

namespace {

constexpr SDL_Color GROUND_COLOR{90, 70, 50, 255};
constexpr double RUN_SPEED = 0.3;  // pixels per millisecond

void addSolid(Game& game, int id, int x, int y, int w, int h, SDL_Color color) {
    auto entity = std::make_shared<Entity>(x, y, w, h, color, 0, 0, false, false, false, id, 0);
    game.addEntity(entity);
    game.addProperty(Property::Collide, id, std::make_shared<CollisionAttribute>());
}

}  // namespace

SDL_Color playerColor(int peerId) {
    static constexpr SDL_Color colors[] = {
        {255, 200, 40, 255},   // 1: yellow
        {60, 200, 255, 255},   // 2: cyan
        {120, 230, 90, 255},   // 3: green
        {230, 110, 230, 255},  // 4: magenta
    };
    return colors[(peerId - 1 + 4) % 4];
}

void build(Game& game) { addSolid(game, 1, 0, GROUND_TOP, WIDTH, HEIGHT - GROUND_TOP, GROUND_COLOR); }

std::shared_ptr<Entity> addPlayer(Game& game, int peerId, bool isLocal) {
    const int id = playerEntityId(peerId);
    auto player = std::make_shared<Entity>(100, GROUND_TOP - 260, PLAYER_SIZE, PLAYER_SIZE,
                                           playerColor(peerId), 0, RUN_SPEED, true, isLocal, true,
                                           id, peerId);
    game.addEntity(player);
    game.addProperty(Property::Collide, id, std::make_shared<CollisionAttribute>());
    game.addProperty(Property::Gravity, id, std::make_shared<GravityAttribute>());
    if (isLocal) {
        game.addProperty(Property::Control, id, std::make_shared<ControlAttribute>());
    }
    return player;
}

}  // namespace arena
