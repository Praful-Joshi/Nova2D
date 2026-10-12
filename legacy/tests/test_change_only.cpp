#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

#include "mesh.hpp"
#include "message.hpp"

using namespace mesh;

namespace {

// Ticks of the 30 Hz state cadence
constexpr int TICKS_PER_SNAPSHOT = TICKS_PER_SECOND / 30;

// Counts the messages each Peer sends, as seen by an extra transport on the network
class Traffic {
public:
    explicit Traffic(InMemoryNetwork& network) : observer_(network) {}

    // Forgets everything sent so far
    void clear() {
        observer_.receive();
        counts_.clear();
    }

    // Takes in what was sent since the last call
    void collect() {
        for (const auto& raw : observer_.receive()) {
            auto message = decodeMessage(raw);
            if (message) ++counts_[{message->sender, message->type()}];
        }
    }

    int count(int sender, MessageType type) {
        collect();
        return counts_[{sender, type}];
    }

private:
    InMemoryTransport observer_;
    std::map<std::pair<int, MessageType>, int> counts_;
};

bool allIn(const std::vector<TagPeer*>& peers, RoundPhase phase) {
    return std::all_of(peers.begin(), peers.end(),
                       [&](TagPeer* peer) { return peer->rounds().phase() == phase; });
}

}  // namespace

TEST_CASE("Once everyone has joined, a player standing still sends no PLAYER_STATE but keeps "
          "sending heartbeats",
          "[legacy][change-only]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    auto peer2 = joinPeer(network, 2, 2);
    std::vector<TagPeer*> both{peer1.get(), peer2.get()};
    Traffic traffic(network);

    // Everyone has joined, so a Round starts; both players land at their SpawnPoints
    REQUIRE(tickUntil(both, [&] { return allIn(both, RoundPhase::PLAYING); },
                      5 * TICKS_PER_SECOND) >= 0);
    tickAll(both, TICKS_PER_SECOND);
    traffic.clear();

    tickAll(both, 3 * TICKS_PER_SECOND);

    for (int id : {1, 2}) {
        INFO("Peer " << id);
        REQUIRE(traffic.count(id, MessageType::PLAYER_STATE) == 0);
        REQUIRE(traffic.count(id, MessageType::HEARTBEAT) >= 11);  // 4 Hz
    }
    // ...and nobody takes the still Peers for departed ones
    REQUIRE(peer1->connectedPeers() == std::vector<int>{2});
    REQUIRE(peer2->connectedPeers() == std::vector<int>{1});

    // Moving sends state again
    for (int i = 0; i < TICKS_PER_SECOND / 2; ++i) {
        peer1->tick(Keys{SDL_SCANCODE_RIGHT});
        peer2->tick(Keys{});
    }
    REQUIRE(traffic.count(1, MessageType::PLAYER_STATE) >= 10);
    REQUIRE(traffic.count(2, MessageType::PLAYER_STATE) == 0);
}

TEST_CASE("Change-only syncing turns on when the player count reaches --players, whatever the "
          "Peer IDs",
          "[legacy][change-only]") {
    InMemoryNetwork network;

    SECTION("Peers 2 and 4 of 2 players: the count is reached") {
        auto peer2 = joinPeer(network, 2, 2);
        auto peer4 = joinPeer(network, 4, 2);
        Traffic traffic(network);
        tickAll({peer2.get(), peer4.get()}, 2 * TICKS_PER_SECOND);  // landed, standing still
        traffic.clear();

        tickAll({peer2.get(), peer4.get()}, TICKS_PER_SECOND);

        REQUIRE(traffic.count(2, MessageType::PLAYER_STATE) == 0);
        REQUIRE(traffic.count(4, MessageType::PLAYER_STATE) == 0);
        REQUIRE(traffic.count(2, MessageType::HEARTBEAT) >= 3);
        REQUIRE(traffic.count(4, MessageType::HEARTBEAT) >= 3);
    }

    SECTION("Peers 1 and 2 of 3 players: still waiting, so full state keeps flowing") {
        auto peer1 = joinPeer(network, 1, 3);
        auto peer2 = joinPeer(network, 2, 3);
        Traffic traffic(network);
        tickAll({peer1.get(), peer2.get()}, 2 * TICKS_PER_SECOND);
        traffic.clear();

        tickAll({peer1.get(), peer2.get()}, TICKS_PER_SECOND);

        // 30 Hz, give or take a send at either end of the second
        REQUIRE(traffic.count(1, MessageType::PLAYER_STATE) >= 29);
        REQUIRE(traffic.count(2, MessageType::PLAYER_STATE) >= 29);
    }
}

TEST_CASE("Every Peer sends its full state at Round start", "[legacy][change-only]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    auto peer2 = joinPeer(network, 2, 2);
    std::vector<TagPeer*> both{peer1.get(), peer2.get()};
    Traffic traffic(network);

    // Round 1 plays out with both players standing still
    REQUIRE(tickUntil(both, [&] { return allIn(both, RoundPhase::RESULTS); },
                      130 * TICKS_PER_SECOND) >= 0);
    REQUIRE(tickUntil(both, [&] { return allIn(both, RoundPhase::COUNTDOWN); },
                      6 * TICKS_PER_SECOND) >= 0);
    REQUIRE(peer1->rounds().round() == 2);
    traffic.clear();

    tickAll(both, TICKS_PER_SNAPSHOT + 1);

    REQUIRE(traffic.count(1, MessageType::PLAYER_STATE) >= 1);
    REQUIRE(traffic.count(2, MessageType::PLAYER_STATE) >= 1);
    REQUIRE(traffic.count(1, MessageType::HOST_STATE) >= 1);
}

TEST_CASE("Requesting full state makes a still Peer send its state once", "[legacy][change-only]") {
    InMemoryNetwork network;
    auto peer2 = joinPeer(network, 2, 2);
    auto peer4 = joinPeer(network, 4, 2);
    std::vector<TagPeer*> both{peer2.get(), peer4.get()};
    Traffic traffic(network);
    tickAll(both, 2 * TICKS_PER_SECOND);
    traffic.clear();
    tickAll(both, TICKS_PER_SECOND / 2);
    REQUIRE(traffic.count(4, MessageType::PLAYER_STATE) == 0);

    // As after Host Migration
    peer4->requestFullState();
    tickAll(both, TICKS_PER_SECOND);

    REQUIRE(traffic.count(4, MessageType::PLAYER_STATE) == 1);
    REQUIRE(traffic.count(2, MessageType::PLAYER_STATE) == 0);
}

TEST_CASE("A Peer that joins late sees players that have been standing still where they are",
          "[legacy][change-only]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 2);
    auto peer2 = joinPeer(network, 2, 2);
    std::vector<TagPeer*> both{peer1.get(), peer2.get()};
    REQUIRE(tickUntil(both, [&] { return allIn(both, RoundPhase::PLAYING); },
                      5 * TICKS_PER_SECOND) >= 0);
    tickAll(both, 2 * TICKS_PER_SECOND);

    // Peer 3 joins as a Spectator; its new remote players start at their SpawnPoints, in the air
    auto peer3 = joinPeer(network, 3, 2);
    std::vector<TagPeer*> all{peer1.get(), peer2.get(), peer3.get()};
    tickAll(all, TICKS_PER_SECOND);

    for (int id : {1, 2}) {
        INFO("Peer " << id);
        auto& owner = id == 1 ? *peer1 : *peer2;
        REQUIRE(playerOf(*peer3, id) != nullptr);
        REQUIRE(playerOf(*peer3, id)->getX() == owner.localPlayer()->getX());
        REQUIRE(playerOf(*peer3, id)->getY() == owner.localPlayer()->getY());
    }
}

TEST_CASE("A remote player that stood still for seconds shows up moving promptly at normal "
          "speed, not sliding over the quiet spell",
          "[legacy][change-only]") {
    InMemoryNetwork network;
    auto peer2 = joinPeer(network, 2, 2);
    auto peer4 = joinPeer(network, 4, 2);

    // Peer 4 lands and stands still for several seconds, sending no state
    tickAll({peer4.get(), peer2.get()}, 5 * TICKS_PER_SECOND);
    const double startX = peer4->localPlayer()->getX();
    REQUIRE(playerOf(*peer2, 4)->getX() == startX);

    // Then runs left for a second; Peer 2 watches
    std::vector<double> track;  // Peer 4's own x, per tick
    std::vector<double> seen;   // Peer 4's x as Peer 2 shows it, per tick
    for (int i = 0; i < TICKS_PER_SECOND; ++i) {
        peer4->tick(Keys{SDL_SCANCODE_LEFT});
        peer2->tick(Keys{});
        track.push_back(peer4->localPlayer()->getX());
        seen.push_back(playerOf(*peer2, 4)->getX());
    }
    REQUIRE(track.back() < startX - 100);

    const auto delayTicks =
        static_cast<std::size_t>(TagNetwork::INTERPOLATION_DELAY_NS / TagPeer::TICK_NS);

    // It starts moving 100 ms late, like every other movement, give or take a snapshot: not
    // straight away, jumping most of the first step ahead of time
    const auto firstMove = static_cast<std::size_t>(
        std::find_if(seen.begin(), seen.end(), [&](double x) { return x != startX; }) -
        seen.begin());
    REQUIRE(firstMove >= delayTicks - TICKS_PER_SNAPSHOT);

    // ...then keeps to its track 100 ms in the past, give or take two snapshots
    double fastestTick = 0;
    for (std::size_t i = 1; i < track.size(); ++i) {
        fastestTick = std::max(fastestTick, std::abs(track[i] - track[i - 1]));
    }
    const double tolerance = fastestTick * 2 * TICKS_PER_SNAPSHOT + 0.5;
    for (std::size_t i = delayTicks; i < track.size(); ++i) {
        INFO("tick " << i);
        REQUIRE(std::abs(seen[i] - track[i - delayTicks]) <= tolerance);
    }
}
