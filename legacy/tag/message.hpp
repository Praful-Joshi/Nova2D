#ifndef TAG_MESSAGE_HPP
#define TAG_MESSAGE_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "round_phase.hpp"

// Typed messages Peers send each other, one JSON object per message, encoded with Boost
// property_tree. Every message carries its type and its sender's Peer ID.
//
// To add a message type: add its body struct (with operator==), append it to MessageBody,
// append its MessageType in the same position, and give it a name, a writer and a reader
// in message.cpp.

// "I'm still here": sent regularly so other Peers can tell a crashed Peer from a quiet one
struct Heartbeat {
    bool operator==(const Heartbeat&) const { return true; }
};

// The sender's own player, in World Coordinates
struct PlayerState {
    double x = 0;
    double y = 0;
    int width = 0;  // shrinking changes the size
    int height = 0;
    double velocityX = 0;
    double velocityY = 0;
    bool active = true;  // false while hidden, e.g. waiting to respawn after a DeathZone

    bool operator==(const PlayerState& other) const {
        return x == other.x && y == other.y && width == other.width && height == other.height &&
               velocityX == other.velocityX && velocityY == other.velocityY &&
               active == other.active;
    }
};

// The sender is quitting cleanly
struct Leaving {
    bool operator==(const Leaving&) const { return true; }
};

// A moving platform as the Host simulates it
struct PlatformState {
    int id = 0;
    double x = 0;
    double y = 0;
    int directionX = 1;
    int directionY = 1;

    bool operator==(const PlatformState& other) const {
        return id == other.id && x == other.x && y == other.y &&
               directionX == other.directionX && directionY == other.directionY;
    }
};

// The shared state the Host has authority over, broadcast regularly
struct HostState {
    RoundPhase phase = RoundPhase::LOBBY;
    int64_t remainingMs = 0;  // Timeline time left in the phase
    int round = 0;
    std::vector<int> participants;  // Peer IDs playing this Round; others are Spectators
    std::vector<PlatformState> platforms;

    bool operator==(const HostState& other) const {
        return phase == other.phase && remainingMs == other.remainingMs &&
               round == other.round && participants == other.participants &&
               platforms == other.platforms;
    }
};

// The Host starts a Round's Countdown with these players
struct RoundStart {
    int round = 0;
    std::vector<int> participants;

    bool operator==(const RoundStart& other) const {
        return round == other.round && participants == other.participants;
    }
};

// The Host ends a Round
struct RoundEnd {
    int round = 0;

    bool operator==(const RoundEnd& other) const { return round == other.round; }
};

using MessageBody =
    std::variant<Heartbeat, PlayerState, Leaving, HostState, RoundStart, RoundEnd>;

// In the same order as MessageBody's alternatives
enum class MessageType { HEARTBEAT, PLAYER_STATE, LEAVING, HOST_STATE, ROUND_START, ROUND_END };

struct Message {
    int sender = 0;  // Peer ID, from 1
    MessageBody body;

    [[nodiscard]] MessageType type() const { return static_cast<MessageType>(body.index()); }
    bool operator==(const Message& other) const {
        return sender == other.sender && body == other.body;
    }
};

std::string encodeMessage(const Message& message);

// The message, or nothing if the input is malformed: not JSON, an unknown type, or a missing
// or ill-typed field. Never throws.
std::optional<Message> decodeMessage(const std::string& json);

#endif
