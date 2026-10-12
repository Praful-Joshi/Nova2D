#ifndef LEGACY_TESTS_MESH_HPP
#define LEGACY_TESTS_MESH_HPP

// Helpers for tests that connect several headless Peers through the in-memory transport

#include <functional>
#include <memory>
#include <vector>

#include "arena.hpp"
#include "in_memory_transport.hpp"
#include "tag_peer.hpp"

namespace mesh {

constexpr int TICKS_PER_SECOND = 60;

inline TagConfig peerConfig(int peerId, int players) {
    TagConfig config;
    config.peerId = peerId;
    config.players = players;
    return config;
}

// A Peer on the in-memory network
inline std::unique_ptr<TagPeer> joinPeer(InMemoryNetwork& network, int peerId, int players) {
    return std::make_unique<TagPeer>(peerConfig(peerId, players),
                                     std::make_shared<InMemoryTransport>(network));
}

// The given Peer's player, as the viewer sees it
inline std::shared_ptr<Entity> playerOf(TagPeer& viewer, int peerId) {
    return viewer.game().getEntity(arena::playerEntityId(peerId));
}

// Ticks every Peer once per tick, in the order given, with no keys held
inline void tickAll(const std::vector<TagPeer*>& peers, int ticks) {
    for (int i = 0; i < ticks; ++i) {
        for (TagPeer* peer : peers) peer->tick(Keys{});
    }
}

// Ticks every Peer until the condition holds, up to maxTicks; returns the ticks taken, or -1
inline int tickUntil(const std::vector<TagPeer*>& peers, const std::function<bool()>& condition,
                     int maxTicks) {
    for (int i = 0; i < maxTicks; ++i) {
        if (condition()) return i;
        tickAll(peers, 1);
    }
    return condition() ? maxTicks : -1;
}

}  // namespace mesh

#endif
