#ifndef TAG_PEER_HPP
#define TAG_PEER_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "collision_handler.hpp"
#include "control_handler.hpp"
#include "death_handler.hpp"
#include "eventmanager.hpp"
#include "game.hpp"
#include "input.hpp"
#include "sideboundary_handler.hpp"
#include "tag_network.hpp"
#include "timeline.hpp"
#include "transport.hpp"

struct TagConfig {
    int peerId = 1;
    int players = 1;
    int windowWidth = 1280;
    int windowHeight = 720;
    // Host name or IP address of other Peers, by Peer ID; the rest default to localhost
    std::map<int, std::string> peerAddresses;

    [[nodiscard]] std::string peerAddress(int otherPeerId) const;
};

// One running instance of Tag. nova2d_tag drives it from SDL and the real clock;
// tests drive it headlessly by passing the held keys and advancing time in fixed ticks.
//
// Threading: game state is only changed inside tick() and leave(), which hold stateMutex().
// Anything that reads game state from elsewhere (rendering) holds stateMutex() too. A
// transport's receive thread never touches game state: tick() drains its inbox.
class TagPeer {
public:
    static constexpr int64_t TICK_NS = 1'000'000'000 / 60;

    // Plays offline
    explicit TagPeer(const TagConfig& config);
    // Plays with the other Peers reachable through the transport
    TagPeer(const TagConfig& config, std::shared_ptr<Transport> transport);
    TagPeer(const TagPeer&) = delete;
    TagPeer& operator=(const TagPeer&) = delete;

    // Advances real time by elapsedNs and runs one frame with the given keys held
    void tick(const Keys& keys, int64_t elapsedNs = TICK_NS);

    Game& game() { return game_; }
    Timeline& timeline() { return timeline_; }
    [[nodiscard]] int peerId() const { return config_.peerId; }
    std::shared_ptr<Entity> localPlayer();

    // Tells the other Peers this Peer is quitting (a clean quit); it then plays offline
    void leave();

    // IDs of the other Peers whose players are in the game, ascending
    [[nodiscard]] std::vector<int> connectedPeers() const { return network_.connectedPeers(); }
    // IDs of the Peers that left or timed out during the last tick
    [[nodiscard]] const std::vector<int>& departures() const { return network_.departures(); }

    // Guards game state; see the threading note above
    std::mutex& stateMutex() { return stateMutex_; }

private:
    TagConfig config_;
    std::mutex stateMutex_;
    int64_t nowNs_ = 0;
    Timeline timeline_;
    EventManager events_;
    CollisionHandler collisionHandler_;
    ControlHandler controlHandler_;
    DeathHandler deathHandler_;
    SideBoundaryHandler sideBoundaryHandler_;
    Game game_;
    TagNetwork network_;
    Keys previousKeys_;

    // True on the tick a key goes down, so holding a key acts once
    bool pressed(const Keys& keys, SDL_Scancode key) const;
    void handlePeerKeys(const Keys& keys);
    // Handles a message for the game rules; players and departures are the network's job
    void handleMessage(const Message& message);
};

#endif
