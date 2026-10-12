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

// The Host's Timeline pause and speed, which every Peer applies to its own Timeline. Sent as
// TIMELINE when the Host changes it, and in every HOST_STATE so later joiners pick it up.
struct TimelineState {
    bool paused = false;
    int64_t tic = 1'000'000;  // nanoseconds per Timeline unit: 500'000 is 2x speed

    bool operator==(const TimelineState& other) const {
        return paused == other.paused && tic == other.tic;
    }
};

// One player's total time as It in the current Round, in Timeline ms
struct ItTime {
    int peerId = 0;
    int64_t ms = 0;

    bool operator==(const ItTime& other) const {
        return peerId == other.peerId && ms == other.ms;
    }
};

// The shared state the Host has authority over, broadcast regularly
struct HostState {
    RoundPhase phase = RoundPhase::LOBBY;
    int64_t remainingMs = 0;  // Timeline time left in the phase
    int round = 0;
    std::vector<int> participants;  // Peer IDs playing this Round; others are Spectators
    std::vector<PlatformState> platforms;
    TimelineState timeline;
    int it = 0;                   // Peer ID of It; 0 if nobody is
    int64_t itFrozenMs = 0;       // Timeline time It stays frozen for
    int immune = 0;               // Peer ID with Tag-back Immunity against It; 0 if nobody
    int64_t immuneMs = 0;         // Timeline time the immunity lasts for
    std::vector<ItTime> itTimes;  // each participant's It time so far

    bool operator==(const HostState& other) const {
        return phase == other.phase && remainingMs == other.remainingMs &&
               round == other.round && participants == other.participants &&
               platforms == other.platforms && timeline == other.timeline &&
               it == other.it && itFrozenMs == other.itFrozenMs && immune == other.immune &&
               immuneMs == other.immuneMs && itTimes == other.itTimes;
    }
};

// The Host starts a Round's Countdown with these players, and the one who starts as It
struct RoundStart {
    int round = 0;
    std::vector<int> participants;
    int it = 0;

    bool operator==(const RoundStart& other) const {
        return round == other.round && participants == other.participants && it == other.it;
    }
};

// The Host ends a Round, with each participant's final It time
struct RoundEnd {
    int round = 0;
    std::vector<ItTime> itTimes;

    bool operator==(const RoundEnd& other) const {
        return round == other.round && itTimes == other.itTimes;
    }
};

// The Host decided a Tagging: It (from) touched a player (to), who is now It
struct Tagged {
    int from = 0;
    int to = 0;

    bool operator==(const Tagged& other) const { return from == other.from && to == other.to; }
};

using MessageBody = std::variant<Heartbeat, PlayerState, Leaving, HostState, RoundStart, RoundEnd,
                                 TimelineState, Tagged>;

// In the same order as MessageBody's alternatives
enum class MessageType {
    HEARTBEAT,
    PLAYER_STATE,
    LEAVING,
    HOST_STATE,
    ROUND_START,
    ROUND_END,
    TIMELINE,
    TAGGED,
};

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
