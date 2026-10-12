#ifndef TAG_MESSAGE_HPP
#define TAG_MESSAGE_HPP

#include <optional>
#include <string>
#include <variant>

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

    bool operator==(const PlayerState& other) const {
        return x == other.x && y == other.y && width == other.width && height == other.height &&
               velocityX == other.velocityX && velocityY == other.velocityY;
    }
};

// The sender is quitting cleanly
struct Leaving {
    bool operator==(const Leaving&) const { return true; }
};

using MessageBody = std::variant<Heartbeat, PlayerState, Leaving>;

// In the same order as MessageBody's alternatives
enum class MessageType { HEARTBEAT, PLAYER_STATE, LEAVING };

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
