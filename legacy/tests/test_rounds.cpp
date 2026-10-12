#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "mesh.hpp"

using namespace mesh;

namespace {

bool allIn(const std::vector<TagPeer*>& peers, RoundPhase phase) {
    for (TagPeer* peer : peers) {
        if (peer->rounds().phase() != phase) return false;
    }
    return true;
}

constexpr double TICK_MS = 1000.0 / TICKS_PER_SECOND;

}  // namespace

TEST_CASE("Peer 1 starts as Host", "[legacy][rounds]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    auto peer2 = joinPeer(network, 2, 2);
    tickAll({peer1.get(), peer2.get()}, 10);

    REQUIRE(peer1->hostId() == 1);
    REQUIRE(peer2->hostId() == 1);
    REQUIRE(peer1->isHost());
    REQUIRE_FALSE(peer2->isHost());
}

TEST_CASE("The Lobby holds until every expected Peer is present, then a Countdown starts on "
          "every Peer",
          "[legacy][rounds]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 3);
    auto peer2 = joinPeer(network, 2, 3);
    tickAll({peer1.get(), peer2.get()}, 2 * TICKS_PER_SECOND);
    REQUIRE(allIn({peer1.get(), peer2.get()}, RoundPhase::LOBBY));

    auto peer3 = joinPeer(network, 3, 3);
    std::vector<TagPeer*> all{peer1.get(), peer2.get(), peer3.get()};
    REQUIRE(tickUntil(all, [&] { return allIn(all, RoundPhase::COUNTDOWN); },
                      TICKS_PER_SECOND) >= 0);
    for (TagPeer* peer : all) {
        REQUIRE(peer->rounds().round() == 1);
        REQUIRE(peer->rounds().participants() == std::vector<int>{1, 2, 3});
    }
}

TEST_CASE("A lone Peer stays in the Lobby to practise", "[legacy][rounds]") {
    TagPeer peer(TagConfig{});
    for (int i = 0; i < 10 * TICKS_PER_SECOND; ++i) peer.tick(Keys{});
    REQUIRE(peer.rounds().phase() == RoundPhase::LOBBY);
}

TEST_CASE("A Round counts down for 3 s, plays for 120 s of Timeline time, shows Results for 5 s, "
          "then counts down again",
          "[legacy][rounds]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    auto peer2 = joinPeer(network, 2, 2);
    std::vector<TagPeer*> both{peer1.get(), peer2.get()};
    REQUIRE(tickUntil(both, [&] { return allIn(both, RoundPhase::COUNTDOWN); }, 60) >= 0);

    const auto phaseLengthMs = [&](RoundPhase phase) {
        return tickUntil(both, [&] { return !allIn(both, phase); }, 200 * TICKS_PER_SECOND) *
               TICK_MS;
    };
    const double countdownMs = phaseLengthMs(RoundPhase::COUNTDOWN);
    REQUIRE(allIn(both, RoundPhase::PLAYING));
    const double playingMs = phaseLengthMs(RoundPhase::PLAYING);
    REQUIRE(allIn(both, RoundPhase::RESULTS));
    const double resultsMs = phaseLengthMs(RoundPhase::RESULTS);
    REQUIRE(allIn(both, RoundPhase::COUNTDOWN));

    // Each phase ends within a couple of ticks of its length on every Peer
    REQUIRE(std::abs(countdownMs - 3'000) <= 3 * TICK_MS);
    REQUIRE(std::abs(playingMs - 120'000) <= 3 * TICK_MS);
    REQUIRE(std::abs(resultsMs - 5'000) <= 3 * TICK_MS);
    REQUIRE(peer1->rounds().round() == 2);
    REQUIRE(peer2->rounds().round() == 2);
}

TEST_CASE("The Round timer counts down on every Peer", "[legacy][rounds]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    auto peer2 = joinPeer(network, 2, 2);
    std::vector<TagPeer*> both{peer1.get(), peer2.get()};
    REQUIRE(tickUntil(both, [&] { return allIn(both, RoundPhase::PLAYING); },
                      5 * TICKS_PER_SECOND) >= 0);

    tickAll(both, 10 * TICKS_PER_SECOND);

    for (TagPeer* peer : both) {
        REQUIRE(std::abs(peer->rounds().remainingMs() - 110'000) <= 3 * TICK_MS);
    }
}

TEST_CASE("Players are placed at their SpawnPoints and held still during the Countdown",
          "[legacy][rounds]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    auto peer2 = joinPeer(network, 2, 2);
    std::vector<TagPeer*> both{peer1.get(), peer2.get()};
    peer1->localPlayer()->setPosition(600, 300);  // wandered off in the Lobby
    REQUIRE(tickUntil(both, [&] { return allIn(both, RoundPhase::COUNTDOWN); }, 60) >= 0);
    tickAll(both, 30);

    auto spawn = peer1->game().getEntity(arena::spawnPointId(1));
    REQUIRE(peer1->localPlayer()->getX() == spawn->getX());
    for (int i = 0; i < 60; ++i) {
        peer1->tick(Keys{SDL_SCANCODE_RIGHT});
        peer2->tick(Keys{});
    }
    REQUIRE(peer1->localPlayer()->getX() == spawn->getX());

    REQUIRE(tickUntil(both, [&] { return allIn(both, RoundPhase::PLAYING); },
                      3 * TICKS_PER_SECOND) >= 0);
    tickAll(both, 2 * TICKS_PER_SECOND + 6);  // past It's 2 s freeze, if Peer 1 is It
    for (int i = 0; i < 30; ++i) {
        peer1->tick(Keys{SDL_SCANCODE_RIGHT});
        peer2->tick(Keys{});
    }
    REQUIRE(peer1->localPlayer()->getX() > spawn->getX() + 50);
}

TEST_CASE("Moving platforms on a Peer that isn't Host match the Host's", "[legacy][rounds]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    tickAll({peer1.get()}, 77);  // Peer 2 starts later, so its own simulation is out of phase
    auto peer2 = joinPeer(network, 2, 2);

    tickAll({peer2.get(), peer1.get()}, 3 * TICKS_PER_SECOND);

    // Within one tick of platform travel (0.08 px/ms) of the Host's last broadcast
    const double tolerance = 0.08 * 2 * TICK_MS + 0.5;
    for (int id : {arena::PLATFORM1_ID, arena::PLATFORM2_ID, arena::ELEVATOR_ID}) {
        INFO("platform " << id);
        REQUIRE(std::abs(peer2->game().getEntity(id)->getX() -
                         peer1->game().getEntity(id)->getX()) <= tolerance);
        REQUIRE(std::abs(peer2->game().getEntity(id)->getY() -
                         peer1->game().getEntity(id)->getY()) <= tolerance);
    }
}

TEST_CASE("A Peer that joins during a Round is a Spectator until the next Countdown",
          "[legacy][rounds]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    auto peer2 = joinPeer(network, 2, 2);
    std::vector<TagPeer*> both{peer1.get(), peer2.get()};
    REQUIRE(tickUntil(both, [&] { return allIn(both, RoundPhase::PLAYING); },
                      5 * TICKS_PER_SECOND) >= 0);

    auto peer3 = joinPeer(network, 3, 2);
    std::vector<TagPeer*> all{peer1.get(), peer2.get(), peer3.get()};
    tickAll(all, TICKS_PER_SECOND);

    REQUIRE(peer3->rounds().phase() == RoundPhase::PLAYING);
    REQUIRE(peer3->rounds().isSpectator(3));
    REQUIRE_FALSE(peer3->localPlayer()->isActive());
    REQUIRE_FALSE(playerOf(*peer1, 3)->isActive());

    REQUIRE(tickUntil(all, [&] { return allIn(all, RoundPhase::COUNTDOWN); },
                      130 * TICKS_PER_SECOND) >= 0);
    tickAll(all, 30);
    REQUIRE_FALSE(peer3->rounds().isSpectator(3));
    REQUIRE(peer3->rounds().participants() == std::vector<int>{1, 2, 3});
    REQUIRE(peer3->localPlayer()->isActive());
    REQUIRE(playerOf(*peer1, 3)->isActive());
}
