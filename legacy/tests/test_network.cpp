#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

#include "arena.hpp"
#include "in_memory_transport.hpp"
#include "message.hpp"
#include "tag_peer.hpp"

namespace {

constexpr int TICKS_PER_SECOND = 60;

TagConfig peerConfig(int peerId, int players) {
    TagConfig config;
    config.peerId = peerId;
    config.players = players;
    return config;
}

// A Peer on the in-memory network
// Expects 4 players, so these Peers stay in the Lobby and Rounds never hold players still
std::unique_ptr<TagPeer> joinPeer(InMemoryNetwork& network, int peerId, int players = 4) {
    return std::make_unique<TagPeer>(peerConfig(peerId, players),
                                     std::make_shared<InMemoryTransport>(network));
}

// The given Peer's player, as the viewer sees it
std::shared_ptr<Entity> playerOf(TagPeer& viewer, int peerId) {
    return viewer.game().getEntity(arena::playerEntityId(peerId));
}

// Ticks every Peer once per tick, in the order given, with no keys held
void tickAll(const std::vector<TagPeer*>& peers, int ticks) {
    for (int i = 0; i < ticks; ++i) {
        for (TagPeer* peer : peers) peer->tick(Keys{});
    }
}

bool isSnapshot(const std::vector<double>& snapshots, double x) {
    return std::find(snapshots.begin(), snapshots.end(), x) != snapshots.end();
}

// Lies strictly between two consecutive snapshots
bool isBetweenSnapshots(const std::vector<double>& snapshots, double x) {
    for (std::size_t i = 1; i < snapshots.size(); ++i) {
        const double low = std::min(snapshots[i - 1], snapshots[i]);
        const double high = std::max(snapshots[i - 1], snapshots[i]);
        if (low < x && x < high) return true;
    }
    return false;
}

}  // namespace

TEST_CASE("A remote player's movement appears on other Peers, interpolated between snapshots",
          "[legacy][network]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1);
    auto peer2 = joinPeer(network, 2);
    InMemoryTransport observer(network);  // sees the snapshots Peer 2 sends

    // Both players land at their spawn points
    tickAll({peer1.get(), peer2.get()}, TICKS_PER_SECOND);
    observer.receive();
    REQUIRE(playerOf(*peer1, 2) != nullptr);
    const double startX = playerOf(*peer1, 2)->getX();

    // Peer 2 runs left for half a second, then stops; Peer 1 watches
    std::vector<double> snapshots;
    std::vector<double> seen;
    for (int i = 0; i < TICKS_PER_SECOND; ++i) {
        peer2->tick(i < TICKS_PER_SECOND / 2 ? Keys{SDL_SCANCODE_LEFT} : Keys{});
        peer1->tick(Keys{});
        seen.push_back(playerOf(*peer1, 2)->getX());
        for (const auto& raw : observer.receive()) {
            auto message = decodeMessage(raw);
            if (message && message->sender == 2 && message->type() == MessageType::PLAYER_STATE) {
                snapshots.push_back(std::get<PlayerState>(message->body).x);
            }
        }
    }

    // Peer 1 ends up seeing exactly where Peer 2's player stopped
    REQUIRE(peer2->localPlayer()->getX() < startX - 100);
    REQUIRE(playerOf(*peer1, 2)->getX() == peer2->localPlayer()->getX());
    REQUIRE(playerOf(*peer1, 2)->getY() == peer2->localPlayer()->getY());

    // ...and on the way it showed positions between the snapshots it received
    const auto interpolated = std::count_if(seen.begin(), seen.end(), [&](double x) {
        return !isSnapshot(snapshots, x) && isBetweenSnapshots(snapshots, x);
    });
    REQUIRE(snapshots.size() >= 10);
    REQUIRE(interpolated >= 10);
}

TEST_CASE("A remote player has its Peer's colour and no local physics", "[legacy][network]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1);
    auto peer2 = joinPeer(network, 2);
    tickAll({peer1.get(), peer2.get()}, 5);

    REQUIRE(playerOf(*peer1, 2) != nullptr);
    const SDL_Color color = playerOf(*peer1, 2)->getColor();
    const SDL_Color expected = arena::playerColor(2);
    REQUIRE((color.r == expected.r && color.g == expected.g && color.b == expected.b));
    REQUIRE_FALSE(peer1->game().entityHasProperty(Property::Gravity, arena::playerEntityId(2)));
    REQUIRE_FALSE(peer1->game().entityHasProperty(Property::Control, arena::playerEntityId(2)));
}

TEST_CASE("A Peer that leaves is removed from the other Peers within one tick",
          "[legacy][network]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1, 3);
    auto peer2 = joinPeer(network, 2, 3);
    auto peer3 = joinPeer(network, 3, 3);
    tickAll({peer1.get(), peer2.get(), peer3.get()}, 30);
    REQUIRE(peer1->connectedPeers() == std::vector<int>{2, 3});
    REQUIRE(peer3->connectedPeers() == std::vector<int>{1, 2});

    peer2->leave();
    tickAll({peer1.get(), peer3.get()}, 1);

    REQUIRE(playerOf(*peer1, 2) == nullptr);
    REQUIRE(playerOf(*peer3, 2) == nullptr);
    REQUIRE(peer1->connectedPeers() == std::vector<int>{3});
    REQUIRE(peer3->connectedPeers() == std::vector<int>{1});
    REQUIRE(peer1->departures() == std::vector<int>{2});

    // A departure is reported on the tick it happens only
    tickAll({peer1.get(), peer3.get()}, 1);
    REQUIRE(peer1->departures().empty());
}

TEST_CASE("A Peer that goes silent is removed after 2 seconds, not before", "[legacy][network]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1);
    auto peer2 = joinPeer(network, 2);
    tickAll({peer1.get(), peer2.get()}, 30);
    REQUIRE(peer1->connectedPeers() == std::vector<int>{2});

    // Peer 2 crashes: it never ticks or sends again
    const int ticksTo1900Ms = TICKS_PER_SECOND * 19 / 10;
    tickAll({peer1.get()}, ticksTo1900Ms);
    REQUIRE(playerOf(*peer1, 2) != nullptr);
    REQUIRE(peer1->connectedPeers() == std::vector<int>{2});

    bool departed = false;
    for (int i = 0; i < TICKS_PER_SECOND / 5 && !departed; ++i) {
        peer1->tick(Keys{});
        departed = peer1->departures() == std::vector<int>{2};
    }
    REQUIRE(departed);
    REQUIRE(playerOf(*peer1, 2) == nullptr);
    REQUIRE(peer1->connectedPeers().empty());
}

TEST_CASE("Peers started in any order end up seeing each other", "[legacy][network]") {
    InMemoryNetwork network;

    // Peer 2 starts first and plays alone for a second
    auto peer2 = joinPeer(network, 2);
    tickAll({peer2.get()}, TICKS_PER_SECOND);
    REQUIRE(peer2->connectedPeers().empty());

    auto peer1 = joinPeer(network, 1);
    tickAll({peer1.get(), peer2.get()}, TICKS_PER_SECOND / 2);

    REQUIRE(peer1->connectedPeers() == std::vector<int>{2});
    REQUIRE(peer2->connectedPeers() == std::vector<int>{1});
    REQUIRE(playerOf(*peer1, 2) != nullptr);
    REQUIRE(playerOf(*peer2, 1) != nullptr);
}

TEST_CASE("A Peer without a transport plays offline", "[legacy][network]") {
    TagPeer peer(TagConfig{});
    peer.tick(Keys{});
    peer.leave();
    peer.tick(Keys{});

    REQUIRE(peer.connectedPeers().empty());
    REQUIRE(peer.localPlayer() != nullptr);
}

TEST_CASE("A player respawning from a pit is hidden on other Peers, then reappears without "
          "streaking across the arena",
          "[legacy][network]") {
    InMemoryNetwork network;
    auto peer1 = joinPeer(network, 1);
    auto peer2 = joinPeer(network, 2);
    tickAll({peer1.get(), peer2.get()}, TICKS_PER_SECOND);

    // Peer 2's player drops into pit 1; its SpawnPoint is at the far right of the arena
    peer2->localPlayer()->setPosition(arena::PIT1_LEFT + 80, arena::GROUND_TOP);

    bool seenHidden = false;
    bool seenBetween = false;
    for (int i = 0; i < 3 * TICKS_PER_SECOND; ++i) {
        tickAll({peer2.get(), peer1.get()}, 1);
        auto remote = playerOf(*peer1, 2);
        if (!remote->isActive()) seenHidden = true;
        const double x = remote->getX();
        if (remote->isActive() && x > arena::PIT1_RIGHT && x < arena::PIT2_RIGHT) {
            seenBetween = true;  // drawn part-way between the pit and the SpawnPoint
        }
    }

    REQUIRE(seenHidden);
    REQUIRE_FALSE(seenBetween);
    REQUIRE(playerOf(*peer1, 2)->isActive());
    REQUIRE(playerOf(*peer1, 2)->getX() == peer2->localPlayer()->getX());
}
