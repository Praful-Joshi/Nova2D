#ifndef TAG_NETWORK_HPP
#define TAG_NETWORK_HPP

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <vector>

#include "game.hpp"
#include "message.hpp"
#include "transport.hpp"

// Keeps one Peer in the mesh. It sends this Peer's player state and heartbeats, adds a
// player for each Peer it hears from, removes it when that Peer leaves or goes silent, and
// moves remote players smoothly between the snapshots they send.
//
// Change-only syncing: once switched on, state messages (PLAYER_STATE, HOST_STATE, ...) go out
// only when their contents changed since the last one of their type, still at most at their
// 30 Hz cadence. Heartbeats keep their rate, so a still Peer isn't taken for a departed one.
//
// All times are real time in nanoseconds (the Peer's tick clock, not Timeline time), so
// pausing the Timeline doesn't stop networking.
class TagNetwork {
public:
    static constexpr int64_t STATE_INTERVAL_NS = 1'000'000'000 / 30;  // PLAYER_STATE at 30 Hz
    static constexpr int64_t HEARTBEAT_INTERVAL_NS = 250'000'000;
    static constexpr int64_t TIMEOUT_NS = 2'000'000'000;  // silence before a Peer is removed
    // Remote players are shown this far in the past, so there's a snapshot on either side
    static constexpr int64_t INTERPOLATION_DELAY_NS = 100'000'000;

    // With no transport, the Peer plays offline
    TagNetwork(int peerId, std::shared_ptr<Transport> transport);

    // Takes in the messages that arrived, adding and removing remote players, and drops
    // Peers that have been silent too long. Returns every valid message from another Peer,
    // for game rules that handle other message types.
    std::vector<Message> receive(Game& game, int64_t nowNs);

    // Places each remote player between the two snapshots around (now - interpolation delay)
    void interpolate(Game& game, int64_t nowNs);

    // Sends this Peer's player state and a heartbeat when they're due
    void sendDue(const Entity* localPlayer, int64_t nowNs);

    // Sends a message from this Peer to all others now
    void send(const MessageBody& body);
    // Sends a state message now, unless change-only syncing is on and it's the same as the
    // last one of its type sent
    void sendState(const MessageBody& body);

    void setChangeOnly(bool on) { changeOnly_ = on; }
    [[nodiscard]] bool changeOnly() const { return changeOnly_; }
    // The next state message of each type is sent whether it changed or not, e.g. so a Peer
    // that just joined, or a newly promoted Host, has complete state. Hearing a new Peer
    // does this automatically.
    void forceFullState() { lastSent_.clear(); }

    // Tells the other Peers this Peer is quitting, then goes offline
    void leave();

    // IDs of the other Peers whose players are in the game, ascending
    [[nodiscard]] std::vector<int> connectedPeers() const;
    // IDs of the Peers that left or timed out during the last receive()
    [[nodiscard]] const std::vector<int>& departures() const { return departed_; }

private:
    struct Snapshot {
        int64_t receivedNs;
        PlayerState state;
    };
    struct RemotePeer {
        int64_t lastHeardNs = 0;
        std::deque<Snapshot> snapshots;  // oldest first
    };

    int peerId_;
    std::shared_ptr<Transport> transport_;
    std::map<int, RemotePeer> remotes_;
    std::vector<int> departed_;
    int64_t nextStateNs_ = 0;
    int64_t nextHeartbeatNs_ = 0;
    bool changeOnly_ = false;
    std::map<MessageType, MessageBody> lastSent_;  // the last state message of each type

    void depart(Game& game, int peerId);
};

#endif
