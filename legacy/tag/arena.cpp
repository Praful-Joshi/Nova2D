#include "arena.hpp"

#include "collisions.hpp"
#include "control.hpp"
#include "deathzone.hpp"
#include "gravity.hpp"
#include "movement.hpp"
#include "platform.hpp"
#include "sideboundary.hpp"
#include "spawnpoint.hpp"

namespace arena {

namespace {

constexpr SDL_Color GROUND_COLOR{90, 70, 50, 255};
constexpr SDL_Color LEDGE_COLOR{140, 110, 80, 255};
constexpr SDL_Color PLATFORM_COLOR{200, 200, 210, 255};
constexpr SDL_Color WALL_COLOR{110, 90, 70, 255};

constexpr int PLATFORM_WIDTH = 120;
constexpr int PLATFORM_HEIGHT = 20;
constexpr double PLATFORM_SPEED = 0.08;  // pixels per Timeline unit
constexpr int SPAWN_Y = 500;
constexpr int SPAWN_X[MAX_PLAYERS] = {150, 2370, 1280, 650};

void addSolid(Game& game, int id, int x, int y, int w, int h, SDL_Color color) {
    auto entity = std::make_shared<Entity>(x, y, w, h, color, 0, 0, false, false, false, id, 0);
    game.addEntity(entity);
    game.addProperty(Property::Collide, id, std::make_shared<CollisionAttribute>());
}

void addMovingPlatform(Game& game, int id, int x, int y, double velY, double velX, int startX,
                       int endX, int startY, int endY) {
    auto platform =
        std::make_shared<Platform>(x, y, PLATFORM_WIDTH, PLATFORM_HEIGHT, PLATFORM_COLOR, velY,
                                   velX, false, id, 0, startX, endX, startY, endY);
    game.addEntity(platform);
    game.addProperty(Property::Move, id, std::make_shared<MovementAttribute>());
    game.addProperty(Property::Collide, id, std::make_shared<CollisionAttribute>());
}

void addSpawnPoint(Game& game, int id, int x, int y) {
    auto spawn = std::make_shared<Entity>(x, y, PLAYER_SIZE, PLAYER_SIZE, SDL_Color{0, 0, 0, 0}, 0,
                                          0, false, false, false, id, 0);
    game.addEntity(spawn);
    game.addProperty(Property::SpawnPoint, id, std::make_shared<SpawnPointAttribute>());
}

// A DeathZone filling a pit from just below the ground line, deep enough that no fall skips it
void addPitDeathZone(Game& game, int id, int left, int right) {
    const int top = GROUND_TOP + PLAYER_SIZE;
    auto zone = std::make_shared<Entity>(left, top, right - left, HEIGHT, SDL_Color{0, 0, 0, 0}, 0,
                                         0, false, false, false, id, 0);
    game.addEntity(zone);
    game.addProperty(Property::DeathZone, id,
                     std::make_shared<DeathZoneAttribute>(spawnPointId(1), RESPAWN_DELAY));
}

// A SideBoundary zone's position is in screen space, not World Coordinates
void addSideBoundary(Game& game, int id, int screenX, bool isLeft) {
    auto zone = std::make_shared<Entity>(screenX, 0, SIDE_ZONE_WIDTH, HEIGHT,
                                         SDL_Color{0, 0, 0, 0}, 0, 0, false, false, false, id, 0);
    game.addEntity(zone);
    game.addProperty(Property::SideBoundary, id, std::make_shared<SideBoundaryAttribute>(isLeft));
}

}  // namespace

SDL_Color playerColor(int peerId) {
    static constexpr SDL_Color colors[MAX_PLAYERS] = {
        {255, 200, 40, 255},   // 1: yellow
        {60, 200, 255, 255},   // 2: cyan
        {120, 230, 90, 255},   // 3: green
        {230, 110, 230, 255},  // 4: magenta
    };
    return colors[(peerId - 1 + MAX_PLAYERS) % MAX_PLAYERS];
}

void build(Game& game) {
    const int groundHeight = HEIGHT - GROUND_TOP;

    // Three ground segments with two pits between them
    addSolid(game, 1, 0, GROUND_TOP, PIT1_LEFT, groundHeight, GROUND_COLOR);
    addSolid(game, 2, PIT1_RIGHT, GROUND_TOP, PIT2_LEFT - PIT1_RIGHT, groundHeight, GROUND_COLOR);
    addSolid(game, 3, PIT2_RIGHT, GROUND_TOP, WIDTH - PIT2_RIGHT, groundHeight, GROUND_COLOR);

    // Invisible walls just outside both ends, so players can't leave the arena
    addSolid(game, 4, -100, -HEIGHT, 100, 2 * HEIGHT, SDL_Color{0, 0, 0, 0});
    addSolid(game, 5, WIDTH, -HEIGHT, 100, 2 * HEIGHT, SDL_Color{0, 0, 0, 0});

    // Horizontal platforms bridging the pits, overlapping the ground edges at each end
    const int bridgeY = GROUND_TOP - 70;
    addMovingPlatform(game, PLATFORM1_ID, PIT1_LEFT - 40, bridgeY, 0, PLATFORM_SPEED,
                      PIT1_LEFT - 40, PIT1_RIGHT - PLATFORM_WIDTH + 40, 0, 0);
    addMovingPlatform(game, PLATFORM2_ID, PIT2_RIGHT - PLATFORM_WIDTH + 40, bridgeY, 0,
                      PLATFORM_SPEED, PIT2_LEFT - 40, PIT2_RIGHT - PLATFORM_WIDTH + 40, 0, 0);

    // Elevator up to the central high ledge
    addMovingPlatform(game, ELEVATOR_ID, ELEVATOR_X, ELEVATOR_TOP_MAX, PLATFORM_SPEED, 0, 0, 0,
                      ELEVATOR_TOP_MIN, ELEVATOR_TOP_MAX);
    addSolid(game, 13, HIGH_LEDGE_X, HIGH_LEDGE_TOP, 220, 20, LEDGE_COLOR);

    // Static ledges at different heights
    addSolid(game, 20, LEDGE_X, LEDGE_TOP, 200, 20, LEDGE_COLOR);
    addSolid(game, 21, 500, 400, 180, 20, LEDGE_COLOR);
    addSolid(game, 22, 1900, 520, 200, 20, LEDGE_COLOR);
    addSolid(game, 23, 2150, 400, 180, 20, LEDGE_COLOR);
    addSolid(game, 24, 1600, 300, 120, 20, LEDGE_COLOR);

    // Wall with a low gap beneath it
    const int wallTop = 300;
    addSolid(game, 30, GAP_WALL_X, wallTop, GAP_WALL_WIDTH, GROUND_TOP - GAP_HEIGHT - wallTop,
             WALL_COLOR);

    for (int peerId = 1; peerId <= MAX_PLAYERS; ++peerId) {
        addSpawnPoint(game, spawnPointId(peerId), SPAWN_X[peerId - 1], SPAWN_Y);
    }

    // Falling into a pit respawns only that player, at their own SpawnPoint
    addPitDeathZone(game, PIT1_DEATHZONE_ID, PIT1_LEFT, PIT1_RIGHT);
    addPitDeathZone(game, PIT2_DEATHZONE_ID, PIT2_LEFT, PIT2_RIGHT);

    // SideBoundary zones at the screen edges scroll the Camera across the arena
    game.getCamera().setWorldSize(WIDTH, HEIGHT);
    const int viewWidth = game.getCamera().getViewWidth();
    addSideBoundary(game, LEFT_SIDE_ZONE_ID, 0, true);
    addSideBoundary(game, RIGHT_SIDE_ZONE_ID, viewWidth - SIDE_ZONE_WIDTH, false);
}

std::shared_ptr<Entity> addPlayer(Game& game, int peerId, bool isLocal) {
    const int id = playerEntityId(peerId);
    auto spawn = game.getEntity(spawnPointId(peerId));
    const int x = spawn ? spawn->getRect().x : 0;
    const int y = spawn ? spawn->getRect().y : 0;

    auto player = std::make_shared<Entity>(x, y, PLAYER_SIZE, PLAYER_SIZE, playerColor(peerId), 0,
                                           0, true, isLocal, true, id, peerId);
    player->setSpawnPointId(spawnPointId(peerId));
    game.addEntity(player);
    game.addProperty(Property::Collide, id, std::make_shared<CollisionAttribute>());
    if (isLocal) {
        game.addProperty(Property::Control, id, std::make_shared<ControlAttribute>());
        game.addProperty(Property::Move, id, std::make_shared<MovementAttribute>());
        game.addProperty(Property::Gravity, id, std::make_shared<GravityAttribute>());
    }
    return player;
}

}  // namespace arena
