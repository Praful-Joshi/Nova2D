#ifndef TAG_PEER_HPP
#define TAG_PEER_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "collision_handler.hpp"
#include "control_handler.hpp"
#include "death_handler.hpp"
#include "eventmanager.hpp"
#include "game.hpp"
#include "input.hpp"
#include "rounds.hpp"
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
    [[nodiscard]] const TagConfig& config() const { return config_; }
    std::shared_ptr<Entity> localPlayer();

    // Tells the other Peers this Peer is quitting (a clean quit); it then plays offline
    void leave();

    // Makes this Peer send its full state (PLAYER_STATE, and HOST_STATE if Host) at the next
    // 30 Hz send, even if change-only syncing would skip it as unchanged
    void requestFullState();

    // IDs of the other Peers whose players are in the game, ascending
    [[nodiscard]] std::vector<int> connectedPeers() const { return network_.connectedPeers(); }
    // IDs of the Peers that left or timed out during the last tick
    [[nodiscard]] const std::vector<int>& departures() const { return network_.departures(); }

    // Guards game state; see the threading note above
    std::mutex& stateMutex() { return stateMutex_; }

    // The Round cycle as this Peer sees it
    [[nodiscard]] const Rounds& rounds() const { return rounds_; }
    // The Peer with authority over shared state: platforms, Rounds, ...
    [[nodiscard]] int hostId() const { return hostId_; }
    [[nodiscard]] bool isHost() const { return hostId_ == config_.peerId; }
    // This Peer and the Peers it can hear, ascending
    [[nodiscard]] std::vector<int> presentPeers() const;

    // The chase as this Peer sees it (the Host's decisions; see tagging.hpp)
    // Peer ID of It, or 0 if nobody is
    [[nodiscard]] int itPeerId() const;
    // Whether the player can't move: It at the start of Playing
    bool isFrozen(int peerId);
    // Whether the player has Tag-back Immunity against It, having just tagged them
    bool isImmune(int peerId);
    // The player's time as It this Round, in Timeline ms; final once the Round is over
    [[nodiscard]] int64_t itTimeMs(int peerId) const;
    // Each participant's It time, leaving out any who have left
    [[nodiscard]] std::vector<ItTime> itTimes() const;
    // The participants with the least It time so far: the winners once the Round is over
    [[nodiscard]] std::vector<int> winners() const;
    // Seeds the Host's random choices, such as who starts as It (random unless seeded)
    void seedRandom(unsigned seed) { random_.seed(seed); }
    // Host only, inside tick(): makes a random participant who is still here It, frozen for
    // 2 s, e.g. when It leaves. HOST_STATE tells the other Peers.
    void reassignIt();

    // Custom Events Tag raises for Round changes and Taggings, on every Peer
    static constexpr const char* ROUND_START = "ROUND_START";
    static constexpr const char* ROUND_END = "ROUND_END";
    static constexpr const char* TAGGED = "TAGGED";

private:
    // Applies Tag's Custom Events to this Peer
    class RoundHandler : public EventHandler {
    public:
        explicit RoundHandler(TagPeer& peer) : peer_(peer) {}
        void onEvent(const Event& event) override { peer_.onRoundEvent(event); }

    private:
        TagPeer& peer_;
    };

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
    Rounds rounds_;
    RoundHandler roundHandler_;
    int hostId_ = 1;
    int64_t nextHostStateNs_ = 0;
    std::mt19937 random_;
    // Host: Playing time already added to someone's It time this Round
    int64_t itAccountedMs_ = 0;
    // Whether applyRoles() blocked the local player's dash and shrink
    bool abilitiesBlockedByRoles_ = false;

    // True on the tick a key goes down, so holding a key acts once
    bool pressed(const Keys& keys, SDL_Scancode key) const;
    void handlePeerKeys(const Keys& keys);
    // Handles a message for the game rules; players and departures are the network's job
    void handleMessage(const Message& message);

    // Host only: starts and ends Rounds and broadcasts shared state
    void runHost();
    void startRound(int round, const std::vector<int>& participants);
    void endRound();
    void sendHostStateDue();
    // Follows the Host's broadcast state
    void followHost(const HostState& state);
    void raiseRoundEvent(const char* name, int round, const std::vector<int>& participants,
                         int it = 0);
    void onRoundEvent(const Event& event);
    // Host, while Playing: adds the time since the last call to It's It time, then decides
    // whether It is touching someone
    void runTagging();
    void accountItTime();
    void raiseTagged(int from, int to);
    // A random one of the given Peers
    int pickRandom(const std::vector<int>& peerIds);
    // Puts the local player at its SpawnPoint, ready to play
    void placeAtSpawn();
    // Applies each player's part in the Round: frozen in the Countdown (and It for 2 s more),
    // hidden if spectating, no dash or shrink while frozen or Tag-back Immune
    void applyRoles();
};

#endif
