#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cmath>
#include <memory>
#include <vector>

#include "mesh.hpp"
#include "tag_player.hpp"

using namespace mesh;

namespace {

constexpr double TICK_MS = 1000.0 / TICKS_PER_SECOND;
// A player standing on the ground
constexpr double GROUND_Y = arena::GROUND_TOP - arena::PLAYER_SIZE;

// Two Peers playing a Round; Peer 1 is Host. The seed only changes who starts as It.
struct TwoPeers {
    InMemoryNetwork network;
    std::unique_ptr<TagPeer> peer1;
    std::unique_ptr<TagPeer> peer2;

    explicit TwoPeers(unsigned seed)
        : peer1(joinPeer(network, 1, 2)), peer2(joinPeer(network, 2, 2)) {
        peer1->seedRandom(seed);
    }

    [[nodiscard]] std::vector<TagPeer*> all() const { return {peer1.get(), peer2.get()}; }
    TagPeer& peer(int peerId) { return peerId == 1 ? *peer1 : *peer2; }

    bool allIn(RoundPhase phase) const {
        return peer1->rounds().phase() == phase && peer2->rounds().phase() == phase;
    }
    bool allSeeIt(int peerId) const {
        return peer1->itPeerId() == peerId && peer2->itPeerId() == peerId;
    }

    // Ticks until the Round is Playing on both Peers, then for the given extra time
    void playFor(double seconds) {
        REQUIRE(tickUntil(all(), [&] { return allIn(RoundPhase::PLAYING); },
                          5 * TICKS_PER_SECOND) >= 0);
        tickAll(all(), static_cast<int>(seconds * TICKS_PER_SECOND));
    }

    // Ticks both Peers, with the keys held on one of them
    void tickHolding(int peerId, const Keys& keys, int ticks) {
        for (int i = 0; i < ticks; ++i) {
            peer1->tick(peerId == 1 ? keys : Keys{});
            peer2->tick(peerId == 2 ? keys : Keys{});
        }
    }
};

int otherPlayer(int peerId) { return peerId == 1 ? 2 : 1; }

// Puts the Peer's own player on the ground at x, standing still
void standAt(TagPeer& peer, double x) {
    auto player = peer.localPlayer();
    player->setPosition(x, GROUND_Y);
    player->setVelocityX(0);
    player->setVelocityY(0);
}

int countIt(TagPeer& viewer) {
    int count = 0;
    for (int peerId = 1; peerId <= arena::MAX_PLAYERS; ++peerId) {
        auto tag = viewer.game().getAttribute<TagPlayerAttribute>(Property::Custom,
                                                                  arena::playerEntityId(peerId));
        if (tag && tag->it) ++count;
    }
    return count;
}

}  // namespace

TEST_CASE("At Round start exactly one player is It on every Peer, and It can't move for the "
          "first 2 s of Playing",
          "[legacy][tagging]") {
    TwoPeers match(GENERATE(1u, 2u, 3u, 4u));
    REQUIRE(tickUntil(match.all(), [&] { return match.allIn(RoundPhase::COUNTDOWN); },
                      TICKS_PER_SECOND) >= 0);
    tickAll(match.all(), 5);

    const int it = match.peer1->itPeerId();
    REQUIRE((it == 1 || it == 2));
    REQUIRE(match.allSeeIt(it));
    for (TagPeer* peer : match.all()) REQUIRE(countIt(*peer) == 1);

    REQUIRE(tickUntil(match.all(), [&] { return match.allIn(RoundPhase::PLAYING); },
                      4 * TICKS_PER_SECOND) >= 0);
    // Each player runs away from the arena's nearer end
    const auto runKey = [](int peerId) {
        return Keys{peerId == 1 ? SDL_SCANCODE_RIGHT : SDL_SCANCODE_LEFT};
    };
    const int runner = otherPlayer(it);
    TagPeer& itPeer = match.peer(it);
    TagPeer& runnerPeer = match.peer(runner);
    const double itStartX = itPeer.localPlayer()->getX();
    const double runnerStartX = runnerPeer.localPlayer()->getX();

    // 1.8 s in: It hasn't moved, the runner has a head start
    for (int i = 0; i < static_cast<int>(1.8 * TICKS_PER_SECOND); ++i) {
        itPeer.tick(runKey(it));
        runnerPeer.tick(runKey(runner));
    }
    REQUIRE(itPeer.localPlayer()->getX() == itStartX);
    REQUIRE(std::abs(runnerPeer.localPlayer()->getX() - runnerStartX) > 300);
    for (TagPeer* peer : match.all()) REQUIRE(peer->isFrozen(it));

    // 2.3 s in: It runs too
    match.tickHolding(it, runKey(it), static_cast<int>(0.5 * TICKS_PER_SECOND));
    REQUIRE(std::abs(itPeer.localPlayer()->getX() - itStartX) > 50);
    for (TagPeer* peer : match.all()) REQUIRE_FALSE(peer->isFrozen(it));
}

TEST_CASE("It touching another player passes It to them on every Peer, and they can't tag "
          "back the player who tagged them for 2 s",
          "[legacy][tagging]") {
    TwoPeers match(GENERATE(1u, 2u, 3u, 4u));
    match.playFor(2.5);
    const int firstIt = match.peer1->itPeerId();
    const int runner = otherPlayer(firstIt);

    // Each Peer moves its own player to the same spot
    standAt(match.peer(firstIt), 400);
    standAt(match.peer(runner), 400);
    REQUIRE(tickUntil(match.all(), [&] { return match.allSeeIt(runner); },
                      TICKS_PER_SECOND) >= 0);
    for (TagPeer* peer : match.all()) {
        REQUIRE(countIt(*peer) == 1);
        REQUIRE(peer->isImmune(firstIt));
        REQUIRE_FALSE(peer->isImmune(runner));
    }

    // Still touching, but the new It can't tag back within 2 s
    tickAll(match.all(), static_cast<int>(1.8 * TICKS_PER_SECOND));
    REQUIRE(match.allSeeIt(runner));

    // Afterwards they can
    REQUIRE(tickUntil(match.all(), [&] { return match.allSeeIt(firstIt); },
                      TICKS_PER_SECOND / 2) >= 0);
    for (TagPeer* peer : match.all()) REQUIRE(peer->isImmune(runner));
}

TEST_CASE("A Tag-back Immune player can't dash or shrink", "[legacy][tagging]") {
    TwoPeers match(GENERATE(1u, 2u));
    match.playFor(2.5);
    const int firstIt = match.peer1->itPeerId();
    const int runner = otherPlayer(firstIt);
    standAt(match.peer(firstIt), 400);
    standAt(match.peer(runner), 400);
    REQUIRE(tickUntil(match.all(), [&] { return match.allSeeIt(runner); },
                      TICKS_PER_SECOND) >= 0);

    // The player who tagged is Immune: holding the shrink keys does nothing
    TagPeer& immunePeer = match.peer(firstIt);
    REQUIRE(immunePeer.isImmune(firstIt));
    match.tickHolding(firstIt, Keys{SDL_SCANCODE_S, SDL_SCANCODE_H}, 30);
    REQUIRE(immunePeer.localPlayer()->getRect().w == arena::PLAYER_SIZE);
}

TEST_CASE("Only the Host's view decides a Tagging", "[legacy][tagging]") {
    TwoPeers match(GENERATE(1u, 2u, 3u, 4u));
    match.playFor(2.5);
    const int it = match.peer1->itPeerId();
    TagPeer& peer2 = *match.peer2;  // not Host

    // Peer 2 moves its player onto Peer 1's, as Peer 2 sees it, while the Host is stalled
    auto peer1Player = playerOf(peer2, 1);
    standAt(peer2, peer1Player->getX());
    for (int i = 0; i < TICKS_PER_SECOND; ++i) {
        peer2.tick(Keys{});
        REQUIRE(std::abs(peer2.localPlayer()->getX() - peer1Player->getX()) < 1);
        REQUIRE(std::abs(peer2.localPlayer()->getY() - peer1Player->getY()) < 1);
        REQUIRE(peer2.itPeerId() == it);
    }

    // Once the Host sees the overlap too, it decides the Tagging
    REQUIRE(tickUntil(match.all(), [&] { return match.allSeeIt(otherPlayer(it)); },
                      TICKS_PER_SECOND) >= 0);
}

TEST_CASE("It-time totals add up to the Round length, and the player It the least wins",
          "[legacy][tagging]") {
    TwoPeers match(GENERATE(1u, 2u));
    match.playFor(30);
    const int firstIt = match.peer1->itPeerId();
    const int runner = otherPlayer(firstIt);

    // Tagged about 30 s in, then they part
    standAt(match.peer(firstIt), 400);
    standAt(match.peer(runner), 400);
    REQUIRE(tickUntil(match.all(), [&] { return match.allSeeIt(runner); },
                      TICKS_PER_SECOND) >= 0);
    const double taggedAtMs =
        static_cast<double>(Rounds::PLAYING_MS - match.peer1->rounds().remainingMs());
    standAt(match.peer(runner), 2300);

    REQUIRE(tickUntil(match.all(), [&] { return match.allIn(RoundPhase::RESULTS); },
                      100 * TICKS_PER_SECOND) >= 0);
    tickAll(match.all(), 5);

    for (TagPeer* peer : match.all()) {
        INFO("Peer " << peer->peerId());
        const double firstItMs = static_cast<double>(peer->itTimeMs(firstIt));
        const double runnerMs = static_cast<double>(peer->itTimeMs(runner));
        REQUIRE(std::abs(firstItMs + runnerMs - 120'000) <= 2 * TICK_MS);
        REQUIRE(std::abs(firstItMs - taggedAtMs) <= 3 * TICK_MS);
        REQUIRE(peer->winners() == std::vector<int>{firstIt});
    }
}

TEST_CASE("A reassigned It is frozen for 2 s on every Peer", "[legacy][tagging]") {
    TwoPeers match(GENERATE(1u, 2u));
    match.playFor(10);

    match.peer1->reassignIt();  // as the Host would when It leaves
    tickAll(match.all(), 5);
    const int it = match.peer1->itPeerId();
    REQUIRE((it == 1 || it == 2));
    REQUIRE(match.allSeeIt(it));
    for (TagPeer* peer : match.all()) REQUIRE(peer->isFrozen(it));

    tickAll(match.all(), 2 * TICKS_PER_SECOND);
    for (TagPeer* peer : match.all()) REQUIRE_FALSE(peer->isFrozen(it));
}

TEST_CASE("It falling into a pit is still It after respawning", "[legacy][tagging]") {
    TwoPeers match(GENERATE(1u, 2u));
    match.playFor(2.5);
    const int it = match.peer1->itPeerId();
    TagPeer& itPeer = match.peer(it);

    // Just below the moving platform, over the first pit
    itPeer.localPlayer()->setPosition(880, arena::GROUND_TOP + 10);
    REQUIRE(tickUntil(match.all(), [&] { return !itPeer.localPlayer()->isActive(); },
                      TICKS_PER_SECOND) >= 0);
    REQUIRE(tickUntil(match.all(), [&] { return itPeer.localPlayer()->isActive(); },
                      3 * TICKS_PER_SECOND) >= 0);
    tickAll(match.all(), TICKS_PER_SECOND / 2);

    REQUIRE(match.allSeeIt(it));
}
