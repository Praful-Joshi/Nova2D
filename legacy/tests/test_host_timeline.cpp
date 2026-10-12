#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <vector>

#include "mesh.hpp"
#include "message.hpp"

using namespace mesh;

namespace {

constexpr int64_t NORMAL_TIC = 1'000'000;
constexpr int64_t HALF_SPEED_TIC = 2'000'000;
constexpr int64_t DOUBLE_SPEED_TIC = 500'000;
constexpr int64_t TICK_MS = 1000 / TICKS_PER_SECOND + 1;  // a tick, rounded up

bool allPlaying(const std::vector<TagPeer*>& peers) {
    for (TagPeer* peer : peers) {
        if (peer->rounds().phase() != RoundPhase::PLAYING) return false;
    }
    return true;
}

// Two Peers in a running Round; Peer 1 is Host
struct TwoPeers {
    InMemoryNetwork network;
    std::unique_ptr<TagPeer> host = joinPeer(network, 1, 2);
    std::unique_ptr<TagPeer> other = joinPeer(network, 2, 2);
    std::vector<TagPeer*> both{host.get(), other.get()};

    void startPlaying() {
        REQUIRE(tickUntil(both, [&] { return allPlaying(both); }, 5 * TICKS_PER_SECOND) >= 0);
        tickAll(both, TICKS_PER_SECOND);
    }

    // One Peer presses a key for one tick and releases it, while the others hold nothing
    void press(TagPeer& presser, SDL_Scancode key) {
        for (Keys keys : {Keys{key}, Keys{}}) {
            for (TagPeer* peer : both) peer->tick(peer == &presser ? keys : Keys{});
        }
    }
};

}  // namespace

TEST_CASE("A TIMELINE survives encoding and decoding unchanged",
          "[legacy][messages][host-timeline]") {
    const Message pausedSlow{1, TimelineState{true, HALF_SPEED_TIC}};
    const Message running{3, TimelineState{false, DOUBLE_SPEED_TIC}};

    REQUIRE(decodeMessage(encodeMessage(pausedSlow)) == std::optional<Message>(pausedSlow));
    REQUIRE(decodeMessage(encodeMessage(running)) == std::optional<Message>(running));
    REQUIRE(decodeMessage(encodeMessage(running))->type() == MessageType::TIMELINE);
}

TEST_CASE("A HOST_STATE carries the Host's Timeline state", "[legacy][messages][host-timeline]") {
    HostState state;
    state.phase = RoundPhase::PLAYING;
    state.round = 2;
    state.participants = {1, 2};
    state.timeline = TimelineState{true, HALF_SPEED_TIC};
    const Message message{1, state};

    const std::optional<Message> decoded = decodeMessage(encodeMessage(message));

    REQUIRE(decoded.has_value());
    REQUIRE(std::get<HostState>(decoded->body).timeline == TimelineState{true, HALF_SPEED_TIC});
    REQUIRE(*decoded == message);
}

TEST_CASE("Malformed TIMELINE messages are rejected without throwing",
          "[legacy][messages][host-timeline]") {
    const char* malformed[] = {
        "{\"type\":\"TIMELINE\",\"sender\":\"1\",\"tic\":\"1000000\"}",  // no paused
        "{\"type\":\"TIMELINE\",\"sender\":\"1\",\"paused\":\"true\"}",  // no tic
        "{\"type\":\"TIMELINE\",\"sender\":\"1\",\"paused\":\"maybe\",\"tic\":\"1000000\"}",
        "{\"type\":\"TIMELINE\",\"sender\":\"1\",\"paused\":\"true\",\"tic\":\"fast\"}",
        "{\"type\":\"TIMELINE\",\"sender\":\"1\",\"paused\":\"true\",\"tic\":\"0\"}",  // no speed
        "{\"type\":\"TIMELINE\",\"sender\":\"1\",\"paused\":\"true\",\"tic\":\"-5\"}",
        // HOST_STATE without its Timeline state
        ("{\"type\":\"HOST_STATE\",\"sender\":\"1\",\"phase\":\"LOBBY\",\"remainingMs\":\"0\","
         "\"round\":\"0\",\"participants\":\"\",\"platforms\":\"\"}"),
    };
    for (const char* input : malformed) {
        INFO(input);
        std::optional<Message> decoded;
        REQUIRE_NOTHROW(decoded = decodeMessage(input));
        REQUIRE_FALSE(decoded.has_value());
    }
}

TEST_CASE("The Host pressing P pauses every Peer's Timeline and the Round timer; pressing it "
          "again resumes them",
          "[legacy][host-timeline]") {
    TwoPeers peers;
    peers.startPlaying();

    peers.press(*peers.host, SDL_SCANCODE_P);
    tickAll(peers.both, 10);
    std::vector<int64_t> timeAtPause;
    std::vector<int64_t> remainingAtPause;
    for (TagPeer* peer : peers.both) {
        REQUIRE(peer->timeline().isPaused());
        timeAtPause.push_back(peer->timeline().getTime());
        remainingAtPause.push_back(peer->rounds().remainingMs());
    }

    tickAll(peers.both, 5 * TICKS_PER_SECOND);
    for (std::size_t i = 0; i < peers.both.size(); ++i) {
        INFO("Peer " << peers.both[i]->peerId());
        REQUIRE(peers.both[i]->timeline().getTime() == timeAtPause[i]);
        REQUIRE(peers.both[i]->rounds().remainingMs() == remainingAtPause[i]);
        REQUIRE(peers.both[i]->rounds().phase() == RoundPhase::PLAYING);
    }

    peers.press(*peers.host, SDL_SCANCODE_P);
    tickAll(peers.both, TICKS_PER_SECOND - 2);  // one second in all, with the press
    for (std::size_t i = 0; i < peers.both.size(); ++i) {
        INFO("Peer " << peers.both[i]->peerId());
        REQUIRE_FALSE(peers.both[i]->timeline().isPaused());
        REQUIRE(std::abs(remainingAtPause[i] - peers.both[i]->rounds().remainingMs() - 1'000) <=
                3 * TICK_MS);
    }
}

TEST_CASE("A Peer that isn't Host pressing P, 1, 2 or 3 changes nothing anywhere",
          "[legacy][host-timeline]") {
    TwoPeers peers;
    peers.startPlaying();
    std::vector<int64_t> remainingBefore;
    for (TagPeer* peer : peers.both) remainingBefore.push_back(peer->rounds().remainingMs());

    // Four presses, then the rest of 10 s
    for (SDL_Scancode key : {SDL_SCANCODE_P, SDL_SCANCODE_1, SDL_SCANCODE_3, SDL_SCANCODE_2}) {
        peers.press(*peers.other, key);
    }
    tickAll(peers.both, 10 * TICKS_PER_SECOND - 8);

    for (std::size_t i = 0; i < peers.both.size(); ++i) {
        INFO("Peer " << peers.both[i]->peerId());
        REQUIRE_FALSE(peers.both[i]->timeline().isPaused());
        REQUIRE(peers.both[i]->timeline().getTic() == NORMAL_TIC);
        REQUIRE(std::abs(remainingBefore[i] - peers.both[i]->rounds().remainingMs() - 10'000) <=
                3 * TICK_MS);
    }
}

TEST_CASE("At 0.5x speed the Round timer runs at half rate on every Peer",
          "[legacy][host-timeline]") {
    TwoPeers peers;
    peers.startPlaying();
    peers.press(*peers.host, SDL_SCANCODE_3);
    std::vector<int64_t> remainingBefore;
    for (TagPeer* peer : peers.both) remainingBefore.push_back(peer->rounds().remainingMs());

    tickAll(peers.both, 10 * TICKS_PER_SECOND);

    for (std::size_t i = 0; i < peers.both.size(); ++i) {
        INFO("Peer " << peers.both[i]->peerId());
        REQUIRE(peers.both[i]->timeline().getTic() == HALF_SPEED_TIC);
        REQUIRE(std::abs(remainingBefore[i] - peers.both[i]->rounds().remainingMs() - 5'000) <=
                3 * TICK_MS);
    }
}

TEST_CASE("The Host pressing 1 runs every Peer at 2x speed, and 2 returns them to normal",
          "[legacy][host-timeline]") {
    TwoPeers peers;
    peers.startPlaying();

    peers.press(*peers.host, SDL_SCANCODE_1);
    std::vector<int64_t> remainingBefore;
    for (TagPeer* peer : peers.both) remainingBefore.push_back(peer->rounds().remainingMs());
    tickAll(peers.both, 5 * TICKS_PER_SECOND);
    for (std::size_t i = 0; i < peers.both.size(); ++i) {
        INFO("Peer " << peers.both[i]->peerId());
        REQUIRE(peers.both[i]->timeline().getTic() == DOUBLE_SPEED_TIC);
        REQUIRE(std::abs(remainingBefore[i] - peers.both[i]->rounds().remainingMs() - 10'000) <=
                3 * TICK_MS);
    }

    peers.press(*peers.host, SDL_SCANCODE_2);
    for (TagPeer* peer : peers.both) REQUIRE(peer->timeline().getTic() == NORMAL_TIC);
}

TEST_CASE("A Peer that joins later gets the Host's pause and speed", "[legacy][host-timeline]") {
    InMemoryNetwork network;
    auto host = joinPeer(network, 1, 3);
    for (Keys keys : {Keys{SDL_SCANCODE_3}, Keys{}, Keys{SDL_SCANCODE_P}, Keys{}}) host->tick(keys);
    REQUIRE(host->timeline().isPaused());

    auto late = joinPeer(network, 2, 3);
    tickAll({host.get(), late.get()}, 10);

    REQUIRE(late->timeline().isPaused());
    REQUIRE(late->timeline().getTic() == HALF_SPEED_TIC);
}

TEST_CASE("Only the Host's TIMELINE messages count", "[legacy][host-timeline]") {
    InMemoryNetwork network;
    auto host = joinPeer(network, 1, 4);
    auto peer3 = joinPeer(network, 3, 4);
    InMemoryTransport impostor(network);  // claims to be Peer 2
    tickAll({host.get(), peer3.get()}, 10);

    impostor.send(encodeMessage(Message{2, TimelineState{true, HALF_SPEED_TIC}}));
    tickAll({host.get(), peer3.get()}, 10);

    for (TagPeer* peer : {host.get(), peer3.get()}) {
        REQUIRE_FALSE(peer->timeline().isPaused());
        REQUIRE(peer->timeline().getTic() == NORMAL_TIC);
    }
}

TEST_CASE("A game paused for longer than the heartbeat timeout keeps every Peer",
          "[legacy][host-timeline]") {
    TwoPeers peers;
    peers.startPlaying();
    peers.press(*peers.host, SDL_SCANCODE_P);

    tickAll(peers.both, 10 * TICKS_PER_SECOND);

    REQUIRE(peers.host->connectedPeers() == std::vector<int>{2});
    REQUIRE(peers.other->connectedPeers() == std::vector<int>{1});
    REQUIRE(playerOf(*peers.host, 2) != nullptr);
    REQUIRE(playerOf(*peers.other, 1) != nullptr);
}
