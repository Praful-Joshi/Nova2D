#include <catch2/catch_test_macros.hpp>

#include "arena.hpp"
#include "tag_peer.hpp"

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
