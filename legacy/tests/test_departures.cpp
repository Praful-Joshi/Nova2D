#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <memory>
#include <vector>

#include "mesh.hpp"

using namespace mesh;

namespace {

constexpr double TICK_MS = 1000.0 / TICKS_PER_SECOND;

// Peers 1-3 on one in-memory network, ticked until the first Round is being played
struct ThreePeers {
    InMemoryNetwork network;
    std::unique_ptr<TagPeer> peer1;
    std::unique_ptr<TagPeer> peer2;
    std::unique_ptr<TagPeer> peer3;

    explicit ThreePeers(unsigned seed) {
        peer1 = joinPeer(network, 1, 3);
        peer2 = joinPeer(network, 2, 3);
        peer3 = joinPeer(network, 3, 3);
        peer1->seedRandom(seed);
        REQUIRE(tickUntil(all(), [&] { return allPlaying(all()); }, 6 * TICKS_PER_SECOND) >= 0);
    }

    std::vector<TagPeer*> all() { return {peer1.get(), peer2.get(), peer3.get()}; }
    TagPeer& peer(int id) { return id == 1 ? *peer1 : id == 2 ? *peer2 : *peer3; }

    static bool allPlaying(const std::vector<TagPeer*>& peers) {
        for (TagPeer* peer : peers) {
            if (peer->rounds().phase() != RoundPhase::PLAYING) return false;
        }
        return true;
    }

    // The remaining Peers after the given one has gone
    std::vector<TagPeer*> without(int id) {
        std::vector<TagPeer*> rest;
        for (TagPeer* peer : all()) {
            if (peer->peerId() != id) rest.push_back(peer);
        }
        return rest;
    }
};

// A seed whose first It is (or isn't) Peer 1
unsigned seedWhereFirstItIs(bool peer1IsIt) {
    for (unsigned seed = 0;; ++seed) {
        ThreePeers probe(seed);
        if ((probe.peer1->itPeerId() == 1) == peer1IsIt) return seed;
    }
}

}  // namespace

TEST_CASE("When It leaves, exactly one remaining player becomes It, frozen for 2 s",
          "[legacy][departures]") {
    for (unsigned seed : {seedWhereFirstItIs(false), 7u, 11u}) {
        ThreePeers peers(seed);
        tickAll(peers.all(), 3 * TICKS_PER_SECOND);  // past the first It's freeze
        const int leaver = peers.peer1->itPeerId();
        REQUIRE(leaver != 0);

        peers.peer(leaver).leave();
        const std::vector<TagPeer*> rest = peers.without(leaver);
        tickAll(rest, 2);

        const int newIt = rest.front()->itPeerId();
        INFO("seed " << seed << ", It " << leaver << " left");
        REQUIRE(newIt != 0);
        REQUIRE(newIt != leaver);
        for (TagPeer* peer : rest) {
            REQUIRE(peer->itPeerId() == newIt);
            REQUIRE(peer->isFrozen(newIt));
        }
        tickAll(rest, static_cast<int>(2.1 * TICKS_PER_SECOND));
        for (TagPeer* peer : rest) REQUIRE_FALSE(peer->isFrozen(newIt));
    }
}

TEST_CASE("When the Host leaves, the lowest remaining Peer becomes Host and the Round carries on",
          "[legacy][departures]") {
    ThreePeers peers(seedWhereFirstItIs(false));
    tickAll(peers.all(), 10 * TICKS_PER_SECOND);
    const int it = peers.peer1->itPeerId();
    const int64_t remainingBefore = peers.peer2->rounds().remainingMs();
    const int64_t itTimeBefore = peers.peer2->itTimeMs(it);
    REQUIRE(itTimeBefore > 0);

    peers.peer1->leave();
    const std::vector<TagPeer*> rest = peers.without(1);
    tickAll(rest, TICKS_PER_SECOND);

    for (TagPeer* peer : rest) {
        REQUIRE(peer->hostId() == 2);
        REQUIRE(peer->rounds().phase() == RoundPhase::PLAYING);
        REQUIRE(peer->itPeerId() == it);
        // One more second of the Round, all of it still counted as It time
        REQUIRE(std::abs(peer->rounds().remainingMs() - (remainingBefore - 1000)) <= 3 * TICK_MS);
        REQUIRE(std::abs(peer->itTimeMs(it) - (itTimeBefore + 1000)) <= 3 * TICK_MS);
    }
    REQUIRE(peers.peer2->isHost());
    REQUIRE_FALSE(peers.peer3->isHost());
}

TEST_CASE("When the Host crashes, Host Migration happens after the 2 s heartbeat timeout",
          "[legacy][departures]") {
    ThreePeers peers(seedWhereFirstItIs(false));
    const int it = peers.peer1->itPeerId();
    const std::vector<TagPeer*> rest = peers.without(1);  // Peer 1 stops without LEAVING

    tickAll(rest, static_cast<int>(1.5 * TICKS_PER_SECOND));
    for (TagPeer* peer : rest) REQUIRE(peer->hostId() == 1);

    tickAll(rest, TICKS_PER_SECOND);
    for (TagPeer* peer : rest) {
        REQUIRE(peer->hostId() == 2);
        REQUIRE(peer->itPeerId() == it);
        REQUIRE(peer->rounds().phase() == RoundPhase::PLAYING);
    }
}

TEST_CASE("When the Host is also It and leaves, Host Migration and It reassignment both happen",
          "[legacy][departures]") {
    ThreePeers peers(seedWhereFirstItIs(true));
    REQUIRE(peers.peer1->itPeerId() == 1);

    peers.peer1->leave();
    const std::vector<TagPeer*> rest = peers.without(1);
    tickAll(rest, 2);

    const int newIt = peers.peer2->itPeerId();
    REQUIRE((newIt == 2 || newIt == 3));
    for (TagPeer* peer : rest) {
        REQUIRE(peer->hostId() == 2);
        REQUIRE(peer->itPeerId() == newIt);
        REQUIRE(peer->isFrozen(newIt));
    }
}

TEST_CASE("Going from 2 players to 1 ends the Round immediately", "[legacy][departures]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    auto peer2 = joinPeer(network, 2, 2);
    std::vector<TagPeer*> both{peer1.get(), peer2.get()};
    REQUIRE(tickUntil(both, [&] { return ThreePeers::allPlaying(both); },
                      6 * TICKS_PER_SECOND) >= 0);
    tickAll(both, 5 * TICKS_PER_SECOND);

    peer2->leave();
    tickAll({peer1.get()}, 2);

    REQUIRE(peer1->rounds().phase() == RoundPhase::RESULTS);
}

TEST_CASE("Every Peer shows who is now Host, and a Round after Host Migration plays normally",
          "[legacy][departures]") {
    ThreePeers peers(seedWhereFirstItIs(false));
    const int round = peers.peer2->rounds().round();
    peers.peer1->leave();
    const std::vector<TagPeer*> rest = peers.without(1);
    tickAll(rest, 2);

    for (TagPeer* peer : rest) REQUIRE(peer->hostChangeNotice() == std::optional<int>(2));
    tickAll(rest, 4 * TICKS_PER_SECOND);
    for (TagPeer* peer : rest) REQUIRE_FALSE(peer->hostChangeNotice().has_value());

    // The Round runs out, Results show, and the next Round starts with the two who are left
    REQUIRE(tickUntil(rest, [&] {
        return rest[0]->rounds().round() == round + 1 && ThreePeers::allPlaying(rest);
    }, 130 * TICKS_PER_SECOND) >= 0);
    for (TagPeer* peer : rest) {
        REQUIRE(peer->rounds().participants() == std::vector<int>{2, 3});
        REQUIRE(peer->itPeerId() == rest[0]->itPeerId());
        REQUIRE(peer->itPeerId() != 0);
    }
}
