#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "arena.hpp"
#include "camera.hpp"
#include "tag_peer.hpp"

using Catch::Approx;

namespace {

void tickFor(TagPeer& peer, int ticks, const Keys& keys = Keys{}) {
    for (int i = 0; i < ticks; ++i) peer.tick(keys);
}

// Stands the local player on the ground at the given World x and lets it settle
void placePlayerOnGround(TagPeer& peer, double x) {
    peer.localPlayer()->setPosition(x, arena::GROUND_TOP - arena::PLAYER_SIZE);
    tickFor(peer, 30);
}

constexpr int VIEW_WIDTH = 1280;
constexpr int MAX_CAMERA_X = arena::WIDTH - VIEW_WIDTH;  // 1280

}  // namespace

TEST_CASE("A Peer's Camera starts at the left end of the arena with the window's view size",
          "[legacy][camera]") {
    TagPeer peer(TagConfig{});

    REQUIRE(peer.game().getCamera().getX() == 0);
    REQUIRE(peer.game().getCamera().getY() == 0);
    REQUIRE(peer.game().getCamera().getViewWidth() == VIEW_WIDTH);
    REQUIRE(peer.game().getCamera().getViewHeight() == 720);
}

TEST_CASE("A Camera is clamped to its world bounds", "[legacy][camera]") {
    Camera camera(1280, 720);
    camera.setWorldSize(2560, 720);

    camera.setPosition(500, 40);
    REQUIRE(camera.getX() == 500);
    REQUIRE(camera.getY() == 0);  // the world is exactly one view tall

    camera.scrollBy(2000, 0);
    REQUIRE(camera.getX() == 1280);

    camera.scrollBy(-5000, 0);
    REQUIRE(camera.getX() == 0);
}

TEST_CASE("Walking into the right SideBoundary zone scrolls the Camera, not the Entities",
          "[legacy][camera]") {
    // Two identical Peers; only one of them walks, so every other Entity (including the moving
    // platforms) must end up at the same World Coordinates in both
    TagPeer walker(TagConfig{});
    TagPeer idle(TagConfig{});
    // On the right-hand ground segment, clear of pits and walls
    placePlayerOnGround(walker, 1800);
    placePlayerOnGround(idle, 1800);
    const double cameraBefore = walker.game().getCamera().getX();
    const double playerBefore = walker.localPlayer()->getX();

    tickFor(walker, 60, Keys{SDL_SCANCODE_RIGHT});  // 1 s at 0.35 px/ms: about 350 px
    tickFor(idle, 60);

    const double walked = walker.localPlayer()->getX() - playerBefore;
    REQUIRE(walked > 300);
    // The player is held at the zone's inner edge, so the Camera follows it pixel for pixel
    REQUIRE(walker.game().getCamera().getX() > cameraBefore);
    REQUIRE(walker.game().getCamera().getX() == Approx(cameraBefore + walked).margin(1));

    for (const auto& [id, entity] : idle.game().getEntities()) {
        if (id == walker.localPlayer()->getEntityID()) continue;
        const auto other = walker.game().getEntity(id);
        REQUIRE(other != nullptr);
        REQUIRE(other->getX() == entity->getX());
        REQUIRE(other->getY() == entity->getY());
        REQUIRE(other->getRect().w == entity->getRect().w);
        REQUIRE(other->getRect().h == entity->getRect().h);
    }
}

TEST_CASE("Walking into the left SideBoundary zone scrolls the Camera back", "[legacy][camera]") {
    TagPeer peer(TagConfig{});
    // Near the right end of the left-hand ground segment, with the view scrolled a little right
    placePlayerOnGround(peer, 700);
    peer.game().getCamera().setPosition(300, 0);
    const double playerBefore = peer.localPlayer()->getX();

    tickFor(peer, 60, Keys{SDL_SCANCODE_LEFT});  // about 350 px left

    // Once inside the zone the player is held at its inner edge, so the Camera scrolled by the
    // distance walked past that edge: the player is now SIDE_ZONE_WIDTH from the screen's left
    const double playerAfter = peer.localPlayer()->getX();
    REQUIRE(playerBefore - playerAfter > 300);
    REQUIRE(peer.game().getCamera().getX() < 300);
    REQUIRE(peer.game().getCamera().getX() ==
            Approx(playerAfter - arena::SIDE_ZONE_WIDTH).margin(1));
}

TEST_CASE("The Camera never scrolls past either end of the arena", "[legacy][camera]") {
    TagPeer peer(TagConfig{});

    // Walk right to the far end of the arena
    placePlayerOnGround(peer, 1800);
    for (int i = 0; i < 300; ++i) {
        peer.tick(Keys{SDL_SCANCODE_RIGHT});
        REQUIRE(peer.game().getCamera().getX() >= 0);
        REQUIRE(peer.game().getCamera().getX() <= MAX_CAMERA_X);
    }
    REQUIRE(peer.localPlayer()->getX() == Approx(arena::WIDTH - arena::PLAYER_SIZE));
    REQUIRE(peer.game().getCamera().getX() == MAX_CAMERA_X);

    // Then walk left to the near end
    placePlayerOnGround(peer, 700);
    for (int i = 0; i < 300; ++i) {
        peer.tick(Keys{SDL_SCANCODE_LEFT});
        REQUIRE(peer.game().getCamera().getX() >= 0);
        REQUIRE(peer.game().getCamera().getX() <= MAX_CAMERA_X);
    }
    REQUIRE(peer.localPlayer()->getX() == Approx(0).margin(1e-6));
    REQUIRE(peer.game().getCamera().getX() == 0);
}

TEST_CASE("Only the local, controllable player scrolls the Camera", "[legacy][camera]") {
    TagPeer peer(TagConfig{});
    auto remote = arena::addPlayer(peer.game(), 2, false);
    remote->setPosition(2400, arena::GROUND_TOP - arena::PLAYER_SIZE);

    tickFor(peer, 30);

    REQUIRE(peer.game().getCamera().getX() == 0);
}
