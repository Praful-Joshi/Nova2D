#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <unordered_map>

#include "arena.hpp"
#include "deathzone.hpp"
#include "tag_peer.hpp"

namespace {

constexpr int64_t MS = 1'000'000;

void tickFor(TagPeer& peer, int ticks, int64_t elapsedNs, const Keys& keys = Keys{}) {
    for (int i = 0; i < ticks; ++i) peer.tick(keys, elapsedNs);
}

// Drops the local player into the first pit, below the bridging platform, and ticks in 10 ms
// steps until the DeathZone hides them
void fallIntoPit(TagPeer& peer) {
    auto player = peer.localPlayer();
    player->setPosition(arena::PIT1_LEFT + 80, arena::GROUND_TOP + 10);
    for (int i = 0; i < 50 && player->isActive(); ++i) peer.tick(Keys{}, 10 * MS);
    REQUIRE_FALSE(player->isActive());
}

// Ticks in 10 ms steps until the hidden local player reappears
void waitForRespawn(TagPeer& peer) {
    for (int i = 0; i < 200 && !peer.localPlayer()->isActive(); ++i) peer.tick(Keys{}, 10 * MS);
    REQUIRE(peer.localPlayer()->isActive());
}

}  // namespace

TEST_CASE("A player who falls into a pit is hidden, then reappears at their own SpawnPoint",
          "[legacy][tag][deathzone]") {
    TagPeer peer(TagConfig{});
    auto player = peer.localPlayer();
    auto spawn = peer.game().getEntity(arena::spawnPointId(1));
    REQUIRE(spawn);

    fallIntoPit(peer);
    const int64_t fellAt = peer.timeline().getTime();
    const double hiddenX = player->getX();
    const double hiddenY = player->getY();

    // 990 ms later: still hidden, and holding keys does nothing
    tickFor(peer, 99, 10 * MS, Keys{SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP});
    REQUIRE(peer.timeline().getTime() == fellAt + 990);
    REQUIRE_FALSE(player->isActive());
    REQUIRE(player->getX() == hiddenX);
    REQUIRE(player->getY() == hiddenY);

    // 1 s after falling: back at their own SpawnPoint, at rest
    peer.tick(Keys{}, 10 * MS);
    REQUIRE(player->isActive());
    REQUIRE(player->getX() == spawn->getX());
    REQUIRE(player->getY() == spawn->getY());
    REQUIRE(player->getVelocityX() == 0);
    REQUIRE(player->getVelocityY() == 0);

    // Still the same Entity
    REQUIRE(peer.localPlayer() == player);
}

TEST_CASE("Each player respawns at their own SpawnPoint", "[legacy][tag][deathzone]") {
    TagPeer peer(TagConfig{3, 3});
    auto spawn = peer.game().getEntity(arena::spawnPointId(3));
    REQUIRE(spawn);

    fallIntoPit(peer);
    waitForRespawn(peer);

    REQUIRE(peer.localPlayer()->getX() == spawn->getX());
    REQUIRE(peer.localPlayer()->getY() == spawn->getY());
}

TEST_CASE("Respawning waits for Timeline time, so pausing delays it", "[legacy][tag][deathzone]") {
    TagPeer peer(TagConfig{});
    auto player = peer.localPlayer();
    fallIntoPit(peer);
    const int64_t fellAt = peer.timeline().getTime();

    tickFor(peer, 50, 10 * MS);
    peer.tick(Keys{SDL_SCANCODE_P}, 10 * MS);  // pauses at 510 ms after falling
    tickFor(peer, 200, 10 * MS);               // 2 s of real time
    REQUIRE_FALSE(player->isActive());

    peer.tick(Keys{SDL_SCANCODE_P}, 10 * MS);  // unpauses
    tickFor(peer, 48, 10 * MS);
    REQUIRE(peer.timeline().getTime() == fellAt + 990);
    REQUIRE_FALSE(player->isActive());

    peer.tick(Keys{}, 10 * MS);
    REQUIRE(player->isActive());
}

TEST_CASE("Only the player who fell respawns; nothing else in the world changes",
          "[legacy][tag][deathzone]") {
    // Two identical worlds; in one of them the local player falls into a pit
    TagPeer faller(TagConfig{});
    TagPeer twin(TagConfig{});
    for (TagPeer* peer : {&faller, &twin}) {
        arena::addPlayer(peer->game(), 2, false);
        // A remote player isn't controlled by this Peer, so a DeathZone here ignores it
        peer->game().getEntity(arena::playerEntityId(2))
            ->setPosition(arena::PIT2_LEFT + 80, arena::GROUND_TOP + 50);
    }

    fallIntoPit(faller);
    const int ticksSoFar = static_cast<int>(faller.timeline().getTime() / 10);
    tickFor(twin, ticksSoFar, 10 * MS);
    tickFor(faller, 150, 10 * MS);  // past the respawn
    tickFor(twin, 150, 10 * MS);
    REQUIRE(faller.timeline().getTime() == twin.timeline().getTime());
    REQUIRE(faller.localPlayer()->isActive());

    const int fallerId = arena::playerEntityId(1);
    for (const auto& [id, entity] : faller.game().getEntities()) {
        if (id == fallerId) continue;
        auto same = twin.game().getEntity(id);
        REQUIRE(same);
        INFO("Entity " << id);
        REQUIRE(entity->getX() == same->getX());
        REQUIRE(entity->getY() == same->getY());
        REQUIRE(entity->isActive());
    }
    auto remote = faller.game().getEntity(arena::playerEntityId(2));
    REQUIRE(remote->getX() == arena::PIT2_LEFT + 80);
    REQUIRE(remote->getY() == arena::GROUND_TOP + 50);
}

TEST_CASE("With the reset-world flag on, every Entity returns to its original position",
          "[legacy][tag][deathzone]") {
    TagPeer peer(TagConfig{});
    Game& game = peer.game();
    arena::addPlayer(game, 2, false);

    std::unordered_map<int, SDL_Rect> original;
    for (const auto& [id, entity] : game.getEntities()) original[id] = entity->getRect();
    game.addProperty(Property::DeathZone, arena::PIT1_DEATHZONE_ID,
                     std::make_shared<DeathZoneAttribute>(arena::spawnPointId(1),
                                                          arena::RESPAWN_DELAY, true, original));

    // Let the platforms move, and move the other player
    tickFor(peer, 100, 10 * MS);
    game.getEntity(arena::playerEntityId(2))->setPosition(2000, 100);
    REQUIRE(game.getEntity(arena::PLATFORM1_ID)->getRect().x !=
            original[arena::PLATFORM1_ID].x);

    fallIntoPit(peer);
    waitForRespawn(peer);

    for (const auto& [id, entity] : game.getEntities()) {
        INFO("Entity " << id);
        REQUIRE(entity->getRect().x == original[id].x);
        REQUIRE(entity->getRect().y == original[id].y);
    }
}
