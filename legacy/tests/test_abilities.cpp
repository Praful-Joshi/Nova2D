#include <catch2/catch_test_macros.hpp>

#include "arena.hpp"
#include "control.hpp"
#include "tag_peer.hpp"

namespace {

constexpr int64_t MS = 1'000'000;

void tickFor(TagPeer& peer, int ticks, const Keys& keys = Keys{}) {
    for (int i = 0; i < ticks; ++i) peer.tick(keys);
}

// Lets the game run with no keys held for the given game time, in 10 ms ticks
void wait(TagPeer& peer, int64_t ms) {
    for (int64_t t = 0; t < ms; t += 10) peer.tick(Keys{}, 10 * MS);
}

// Presses and releases a key without letting any game time pass, so running doesn't move the
// player and only the key-down edge counts
void tap(TagPeer& peer, SDL_Scancode key) {
    peer.tick(Keys{key}, 0);
    peer.tick(Keys{}, 0);
}

// Two taps of the same key 100 ms apart: well inside the 250 ms double-tap window
void doubleTap(TagPeer& peer, SDL_Scancode key) {
    tap(peer, key);
    wait(peer, 100);
    tap(peer, key);
}

// Holds S and H together for one instant
void shrink(TagPeer& peer) { peer.tick(Keys{SDL_SCANCODE_S, SDL_SCANCODE_H}, 0); }

// A Peer whose player has landed and stands still on the ground at the given x
TagPeer& settle(TagPeer& peer, int x) {
    peer.localPlayer()->setPosition(x, arena::GROUND_TOP - arena::PLAYER_SIZE - 10);
    wait(peer, 500);
    return peer;
}

std::shared_ptr<ControlAttribute> controlOf(Game& game, int entityId) {
    return game.getAttribute<ControlAttribute>(Property::Control, entityId);
}

}  // namespace

TEST_CASE("Double-tapping a direction dashes the player 200 px that way", "[tag][abilities]") {
    TagPeer peer(TagConfig{});
    settle(peer, 150);

    doubleTap(peer, SDL_SCANCODE_RIGHT);

    REQUIRE(peer.localPlayer()->getRect().x == 350);
    REQUIRE(peer.localPlayer()->getRect().y == arena::GROUND_TOP - arena::PLAYER_SIZE);
}

TEST_CASE("A second dash within the 10 s cooldown does nothing, and works again after it",
          "[tag][abilities]") {
    TagPeer peer(TagConfig{});
    settle(peer, 150);
    doubleTap(peer, SDL_SCANCODE_RIGHT);
    REQUIRE(peer.localPlayer()->getRect().x == 350);

    wait(peer, 9'500);
    doubleTap(peer, SDL_SCANCODE_RIGHT);  // second tap lands 9.6 s after the dash
    REQUIRE(peer.localPlayer()->getRect().x == 350);

    wait(peer, 400);
    doubleTap(peer, SDL_SCANCODE_RIGHT);  // 10.1 s after the dash
    REQUIRE(peer.localPlayer()->getRect().x == 550);
}

TEST_CASE("Taps further apart than 250 ms don't dash", "[tag][abilities]") {
    TagPeer peer(TagConfig{});
    settle(peer, 150);

    tap(peer, SDL_SCANCODE_RIGHT);
    wait(peer, 300);
    tap(peer, SDL_SCANCODE_RIGHT);

    REQUIRE(peer.localPlayer()->getRect().x == 150);
}

TEST_CASE("A dash into a solid is blocked and doesn't use up the cooldown", "[tag][abilities]") {
    TagPeer peer(TagConfig{});
    // 200 px right of x = 1280 is x = 1480, inside the low-gap wall (x 1460..1500, down to y 630)
    settle(peer, 1280);

    doubleTap(peer, SDL_SCANCODE_RIGHT);
    REQUIRE(peer.localPlayer()->getRect().x == 1280);

    settle(peer, 150);
    doubleTap(peer, SDL_SCANCODE_RIGHT);
    REQUIRE(peer.localPlayer()->getRect().x == 350);
}

TEST_CASE("A dash past an arena edge stops at the edge", "[tag][abilities]") {
    SECTION("Left edge") {
        TagPeer peer(TagConfig{});
        settle(peer, 150);
        doubleTap(peer, SDL_SCANCODE_LEFT);
        REQUIRE(peer.localPlayer()->getRect().x == 0);
    }
    SECTION("Right edge") {
        TagPeer peer(TagConfig{});
        settle(peer, 2400);
        doubleTap(peer, SDL_SCANCODE_RIGHT);
        REQUIRE(peer.localPlayer()->getRect().x == arena::WIDTH - arena::PLAYER_SIZE);
    }
}

TEST_CASE("Holding S and H shrinks the player to half size for 5 s, feet on the ground",
          "[tag][abilities]") {
    TagPeer peer(TagConfig{});
    settle(peer, 150);

    shrink(peer);
    const SDL_Rect shrunk = peer.localPlayer()->getRect();
    REQUIRE(shrunk.w == 20);
    REQUIRE(shrunk.h == 20);
    REQUIRE(shrunk.x == 160);  // shrinks about the bottom centre
    REQUIRE(shrunk.y + shrunk.h == arena::GROUND_TOP);

    wait(peer, 4'990);
    REQUIRE(peer.localPlayer()->getRect().w == 20);

    wait(peer, 10);
    const SDL_Rect grown = peer.localPlayer()->getRect();
    REQUIRE(grown.w == 40);
    REQUIRE(grown.h == 40);
    REQUIRE(grown.x == 150);
    REQUIRE(grown.y + grown.h == arena::GROUND_TOP);
}

TEST_CASE("Shrinking again within 8 s of the shrink ending does nothing", "[tag][abilities]") {
    TagPeer peer(TagConfig{});
    settle(peer, 150);
    shrink(peer);
    wait(peer, 5'000);
    REQUIRE(peer.localPlayer()->getRect().w == 40);

    wait(peer, 7'900);
    shrink(peer);
    REQUIRE(peer.localPlayer()->getRect().w == 40);

    wait(peer, 100);
    shrink(peer);
    REQUIRE(peer.localPlayer()->getRect().w == 20);
}

TEST_CASE("A shrunk player passes through the low gap and a full-size player can't",
          "[tag][abilities]") {
    const Keys right{SDL_SCANCODE_RIGHT};

    TagPeer full(TagConfig{});
    settle(full, 1280);
    tickFor(full, 40, right);  // about 667 ms of running: 233 px
    REQUIRE(full.localPlayer()->getRect().x + arena::PLAYER_SIZE == arena::GAP_WALL_X);

    TagPeer small(TagConfig{});
    settle(small, 1280);
    shrink(small);
    tickFor(small, 40, right);
    REQUIRE(small.localPlayer()->getRect().x > arena::GAP_WALL_X + arena::GAP_WALL_WIDTH);
}

TEST_CASE("A shrink that ends under the low-gap wall waits for room before growing back",
          "[tag][abilities]") {
    TagPeer peer(TagConfig{});
    settle(peer, 1280);
    shrink(peer);
    peer.localPlayer()->setPosition(1470, arena::GROUND_TOP - 20);  // right under the wall

    wait(peer, 6'000);
    SDL_Rect rect = peer.localPlayer()->getRect();
    REQUIRE(rect.w == 20);
    REQUIRE(rect.x == 1470);
    REQUIRE(rect.y + rect.h == arena::GROUND_TOP);

    tickFor(peer, 10, Keys{SDL_SCANCODE_RIGHT});  // about 58 px: out from under the wall
    wait(peer, 100);
    rect = peer.localPlayer()->getRect();
    REQUIRE(rect.w == 40);
    REQUIRE(rect.h == 40);
    REQUIRE(rect.x >= arena::GAP_WALL_X + arena::GAP_WALL_WIDTH);
    REQUIRE(rect.y + rect.h == arena::GROUND_TOP);
}

TEST_CASE("Two Peers' players have independent abilities", "[tag][abilities]") {
    TagPeer first(TagConfig{1, 2, 1280, 720, {}});
    TagPeer second(TagConfig{2, 2, 1280, 720, {}});
    settle(first, 150);
    settle(second, 2000);

    doubleTap(first, SDL_SCANCODE_RIGHT);
    shrink(first);
    REQUIRE(first.localPlayer()->getRect().w == 20);

    doubleTap(second, SDL_SCANCODE_LEFT);
    REQUIRE(second.localPlayer()->getRect().x == 1800);
    REQUIRE(second.localPlayer()->getRect().w == 40);
}

TEST_CASE("One shared handler keeps two controlled players' abilities independent",
          "[tag][abilities]") {
    TagPeer peer(TagConfig{1, 2, 1280, 720, {}});
    settle(peer, 150);
    auto first = peer.localPlayer();
    auto second = arena::addPlayer(peer.game(), 2, true);
    second->setPosition(2000, arena::GROUND_TOP - arena::PLAYER_SIZE);
    wait(peer, 100);

    // Both see the same keys, and both dash
    doubleTap(peer, SDL_SCANCODE_RIGHT);
    REQUIRE(first->getRect().x == 350);
    REQUIRE(second->getRect().x == 2200);

    // A third player who hasn't dashed yet isn't held back by the others' cooldowns
    auto third = arena::addPlayer(peer.game(), 3, true);
    third->setPosition(1100, arena::GROUND_TOP - arena::PLAYER_SIZE);
    wait(peer, 100);
    doubleTap(peer, SDL_SCANCODE_LEFT);
    REQUIRE(first->getRect().x == 350);
    REQUIRE(second->getRect().x == 2200);
    REQUIRE(third->getRect().x == 900);
}

TEST_CASE("Blocked abilities do nothing", "[tag][abilities]") {
    TagPeer peer(TagConfig{});
    settle(peer, 150);
    auto control = controlOf(peer.game(), peer.localPlayer()->getEntityID());
    control->setAbilitiesBlocked(true);

    doubleTap(peer, SDL_SCANCODE_RIGHT);
    shrink(peer);
    REQUIRE(peer.localPlayer()->getRect().x == 150);
    REQUIRE(peer.localPlayer()->getRect().w == 40);

    control->setAbilitiesBlocked(false);
    wait(peer, 300);
    doubleTap(peer, SDL_SCANCODE_RIGHT);
    REQUIRE(peer.localPlayer()->getRect().x == 350);
}

TEST_CASE("Readiness empties when an ability is used and refills as it becomes ready",
          "[tag][abilities]") {
    TagPeer peer(TagConfig{});
    settle(peer, 150);
    auto control = controlOf(peer.game(), peer.localPlayer()->getEntityID());
    auto now = [&peer] { return peer.timeline().getTime(); };
    REQUIRE(control->dashReadiness(now()) == 1.0);
    REQUIRE(control->shrinkReadiness(now()) == 1.0);

    doubleTap(peer, SDL_SCANCODE_RIGHT);
    shrink(peer);
    REQUIRE(control->dashReadiness(now()) == 0.0);
    REQUIRE(control->shrinkReadiness(now()) == 0.0);

    wait(peer, 5'000);  // dash half way through its 10 s; shrink just ended
    REQUIRE(control->dashReadiness(now()) == 0.5);
    REQUIRE(control->shrinkReadiness(now()) == 0.0);

    wait(peer, 4'000);  // shrink half way through its 8 s cooldown
    REQUIRE(control->shrinkReadiness(now()) == 0.5);

    wait(peer, 4'000);
    REQUIRE(control->dashReadiness(now()) == 1.0);
    REQUIRE(control->shrinkReadiness(now()) == 1.0);
}
