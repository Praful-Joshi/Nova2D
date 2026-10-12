#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "arena.hpp"
#include "collisions.hpp"
#include "movement.hpp"
#include "platform.hpp"
#include "tag_peer.hpp"

using Catch::Approx;

namespace {

void tickFor(TagPeer& peer, int ticks, const Keys& keys = Keys{}) {
    for (int i = 0; i < ticks; ++i) peer.tick(keys);
}

int bottomOf(const SDL_Rect& rect) { return rect.y + rect.h; }

}  // namespace

TEST_CASE("A player above the ground falls under gravity and comes to rest on it",
          "[legacy][physics]") {
    TagPeer peer(TagConfig{});
    const int startY = peer.localPlayer()->getRect().y;
    REQUIRE(bottomOf(peer.localPlayer()->getRect()) < arena::GROUND_TOP);

    tickFor(peer, 10);
    REQUIRE(peer.localPlayer()->getRect().y > startY);

    tickFor(peer, 300);
    REQUIRE(bottomOf(peer.localPlayer()->getRect()) == arena::GROUND_TOP);

    tickFor(peer, 30);
    REQUIRE(bottomOf(peer.localPlayer()->getRect()) == arena::GROUND_TOP);
}

TEST_CASE("A player with no keys held doesn't drift sideways", "[legacy][physics]") {
    TagPeer peer(TagConfig{});
    const int startX = peer.localPlayer()->getRect().x;

    tickFor(peer, 120);

    REQUIRE(peer.localPlayer()->getRect().x == startX);
}

namespace {

constexpr int64_t MS = 1'000'000;

std::shared_ptr<Platform> addPlatform(Game& game, int id, int x, int y, double velY, double velX,
                                      int startX, int endX, int startY, int endY) {
    auto platform = std::make_shared<Platform>(x, y, 100, 20, SDL_Color{200, 200, 200, 255}, velY,
                                               velX, false, id, 0, startX, endX, startY, endY);
    game.addEntity(platform);
    game.addProperty(Property::Move, id, std::make_shared<MovementAttribute>());
    game.addProperty(Property::Collide, id, std::make_shared<CollisionAttribute>());
    return platform;
}

}  // namespace

TEST_CASE("Every moving Entity advances by its own time step in a tick", "[legacy][physics]") {
    TagPeer peer(TagConfig{});
    std::vector<std::shared_ptr<Platform>> platforms;
    for (int i = 0; i < 50; ++i) {
        platforms.push_back(
            addPlatform(peer.game(), 5000 + i, 0, 10 * i, 0, 0.1, 0, 2000, 0, 0));
    }

    peer.tick(Keys{}, 20 * MS);  // 20 ms of game time at 0.1 px/ms is 2 px

    for (const auto& platform : platforms) REQUIRE(platform->getX() == Approx(2.0));
}

TEST_CASE("Gravity follows Timeline time rather than real time", "[legacy][physics]") {
    TagPeer normal(TagConfig{});
    TagPeer halfSpeed(TagConfig{});
    halfSpeed.timeline().changeTic(2'000'000);
    normal.localPlayer()->setPosition(150, 0);
    halfSpeed.localPlayer()->setPosition(150, 0);

    for (int i = 0; i < 20; ++i) normal.tick(Keys{}, 20 * MS);
    for (int i = 0; i < 20; ++i) halfSpeed.tick(Keys{}, 20 * MS);
    REQUIRE(halfSpeed.localPlayer()->getY() < normal.localPlayer()->getY());

    // The same amount of game time gives the same fall
    for (int i = 0; i < 20; ++i) halfSpeed.tick(Keys{}, 20 * MS);
    REQUIRE(halfSpeed.localPlayer()->getY() ==
            Approx(normal.localPlayer()->getY()).margin(0.5));
}

TEST_CASE("A player lands on a ledge and can jump only while standing on something",
          "[legacy][physics]") {
    TagPeer peer(TagConfig{});
    auto player = peer.localPlayer();
    player->setPosition(arena::LEDGE_X + 20, arena::LEDGE_TOP - 100);

    tickFor(peer, 120);
    REQUIRE(bottomOf(player->getRect()) == arena::LEDGE_TOP);

    peer.tick(Keys{SDL_SCANCODE_UP});
    tickFor(peer, 20);  // past the 250 ms double-tap (dash) window, still in the air
    const double airborneY = player->getY();
    REQUIRE(airborneY < arena::LEDGE_TOP - arena::PLAYER_SIZE - 50);

    // A second press in mid-air changes nothing: compare with a twin who doesn't press it
    TagPeer twin(TagConfig{});
    twin.localPlayer()->setPosition(arena::LEDGE_X + 20, arena::LEDGE_TOP - 100);
    tickFor(twin, 120);
    twin.tick(Keys{SDL_SCANCODE_UP});
    tickFor(twin, 20);

    peer.tick(Keys{SDL_SCANCODE_UP});
    twin.tick(Keys{});
    tickFor(peer, 5);
    tickFor(twin, 5);
    REQUIRE(player->getY() == Approx(twin.localPlayer()->getY()));
}

TEST_CASE("Moving platforms turn around at both limits", "[legacy][physics]") {
    TagPeer peer(TagConfig{});
    auto horizontal = addPlatform(peer.game(), 6000, 300, 100, 0, 0.2, 300, 500, 0, 0);
    auto vertical = addPlatform(peer.game(), 6001, 700, 100, 0.2, 0, 0, 0, 100, 300);

    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
    for (int i = 0; i < 240; ++i) {  // 4 s: two full trips at 0.2 px/ms over 200 px
        peer.tick(Keys{});
        minX = std::min(minX, horizontal->getX());
        maxX = std::max(maxX, horizontal->getX());
        minY = std::min(minY, vertical->getY());
        maxY = std::max(maxY, vertical->getY());
    }

    // Never past a limit, and within one tick's travel (0.2 px/ms * 17 ms) of each
    const double oneTick = 0.2 * 17;
    REQUIRE(minX >= 300);
    REQUIRE(minX <= 300 + oneTick);
    REQUIRE(maxX <= 500);
    REQUIRE(maxX >= 500 - oneTick);
    REQUIRE(minY >= 100);
    REQUIRE(minY <= 100 + oneTick);
    REQUIRE(maxY <= 300);
    REQUIRE(maxY >= 300 - oneTick);
    REQUIRE(horizontal->getX() < 500);  // came back from the far limit
    REQUIRE(vertical->getY() < 300);
    REQUIRE(horizontal->getY() == 100);  // and never left their own axis
    REQUIRE(vertical->getX() == 700);
}

TEST_CASE("Holding T toggles proportional scaling exactly once", "[legacy][physics]") {
    TagPeer peer(TagConfig{});
    REQUIRE_FALSE(peer.game().getIsProportionalScaling());

    tickFor(peer, 10, Keys{SDL_SCANCODE_T});
    REQUIRE(peer.game().getIsProportionalScaling());

    tickFor(peer, 2);
    tickFor(peer, 10, Keys{SDL_SCANCODE_T});
    REQUIRE_FALSE(peer.game().getIsProportionalScaling());
}

TEST_CASE("A player standing on the elevator rides it up to the high ledge", "[legacy][physics]") {
    TagPeer peer(TagConfig{});
    auto player = peer.localPlayer();
    auto elevator = peer.game().getEntity(arena::ELEVATOR_ID);
    player->setPosition(arena::ELEVATOR_X + 40, arena::ELEVATOR_TOP_MAX - 100);

    double highest = player->getY();
    for (int i = 0; i < 600; ++i) {  // 10 s: more than a full elevator round trip
        peer.tick(Keys{});
        highest = std::min(highest, player->getY());
    }

    // At the top of its travel the player stood level with the high ledge
    REQUIRE(highest == Approx(arena::HIGH_LEDGE_TOP - arena::PLAYER_SIZE).margin(2));
}

TEST_CASE("Holding Right runs right, and releasing it stops the player", "[legacy][physics]") {
    TagPeer peer(TagConfig{});
    auto player = peer.localPlayer();
    tickFor(peer, 60);  // land first
    const double startX = player->getX();

    for (int i = 0; i < 10; ++i) peer.tick(Keys{SDL_SCANCODE_RIGHT}, 20 * MS);
    REQUIRE(player->getX() == Approx(startX + 0.35 * 200));  // 0.35 px/ms for 200 ms

    const double stoppedX = player->getX();
    tickFor(peer, 10);
    REQUIRE(player->getX() == Approx(stoppedX));
}

TEST_CASE("Players can't walk off either end of the arena", "[legacy][physics]") {
    TagPeer peer(TagConfig{});
    auto player = peer.localPlayer();
    tickFor(peer, 60);

    tickFor(peer, 120, Keys{SDL_SCANCODE_LEFT});  // 2 s: far enough to pass the left end
    REQUIRE(player->getX() == Approx(0).margin(1e-6));
    REQUIRE(bottomOf(player->getRect()) == arena::GROUND_TOP);

    player->setPosition(arena::WIDTH - 200, arena::GROUND_TOP - arena::PLAYER_SIZE);
    tickFor(peer, 120, Keys{SDL_SCANCODE_RIGHT});
    REQUIRE(player->getX() == Approx(arena::WIDTH - arena::PLAYER_SIZE));
    REQUIRE(bottomOf(player->getRect()) == arena::GROUND_TOP);
}
