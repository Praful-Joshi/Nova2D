#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>

#include "message.hpp"

TEST_CASE("A HEARTBEAT survives encoding and decoding unchanged", "[legacy][messages]") {
    const Message heartbeat{3, Heartbeat{}};

    const std::optional<Message> decoded = decodeMessage(encodeMessage(heartbeat));

    REQUIRE(decoded.has_value());
    REQUIRE(decoded->type() == MessageType::HEARTBEAT);
    REQUIRE(*decoded == heartbeat);
}

TEST_CASE("A PLAYER_STATE survives encoding and decoding unchanged", "[legacy][messages]") {
    // Exact positions aren't whole pixels, and shrinking changes the size
    const Message state{2, PlayerState{1234.5678901234567, 619.999, 20, 25, -0.35, 0.8125}};

    const std::optional<Message> decoded = decodeMessage(encodeMessage(state));

    REQUIRE(decoded.has_value());
    REQUIRE(decoded->type() == MessageType::PLAYER_STATE);
    REQUIRE(*decoded == state);
}

TEST_CASE("A LEAVING survives encoding and decoding unchanged", "[legacy][messages]") {
    const Message leaving{4, Leaving{}};

    const std::optional<Message> decoded = decodeMessage(encodeMessage(leaving));

    REQUIRE(decoded.has_value());
    REQUIRE(decoded->type() == MessageType::LEAVING);
    REQUIRE(*decoded == leaving);
}

TEST_CASE("Each message is one line of JSON naming its type and sender", "[legacy][messages]") {
    const std::string json = encodeMessage(Message{1, Heartbeat{}});

    REQUIRE(json.find('\n') == std::string::npos);
    REQUIRE(json.find("\"type\":\"HEARTBEAT\"") != std::string::npos);
    REQUIRE(json.find("\"sender\":\"1\"") != std::string::npos);
}

TEST_CASE("Malformed messages are rejected without throwing", "[legacy][messages]") {
    const char* malformed[] = {
        "",
        "not json",
        "{\"type\":\"HEARTBEAT\",\"sender\":\"1\"",                  // truncated
        "[\"HEARTBEAT\", 1]",                                         // not an object
        "{\"sender\":\"1\"}",                                         // no type
        "{\"type\":\"HEARTBEAT\"}",                                   // no sender
        "{\"type\":\"DANCE\",\"sender\":\"1\"}",                      // unknown type
        "{\"type\":\"HEARTBEAT\",\"sender\":\"one\"}",                // ill-typed sender
        "{\"type\":\"HEARTBEAT\",\"sender\":\"1.5\"}",                // sender isn't whole
        "{\"type\":\"HEARTBEAT\",\"sender\":\"0\"}",                  // Peer IDs start at 1
        "{\"type\":{\"name\":\"HEARTBEAT\"},\"sender\":\"1\"}",       // type isn't a string
        "{\"type\":\"PLAYER_STATE\",\"sender\":\"2\"}",               // no state
        "{\"type\":\"PLAYER_STATE\",\"sender\":\"2\",\"x\":\"10\",\"y\":\"20\",\"width\":\"40\","
        "\"height\":\"40\",\"velocityX\":\"0\"}",  // no velocityY
        "{\"type\":\"PLAYER_STATE\",\"sender\":\"2\",\"x\":\"left\",\"y\":\"20\",\"width\":\"40\","
        "\"height\":\"40\",\"velocityX\":\"0\",\"velocityY\":\"0\"}",  // ill-typed x
        "{\"type\":\"PLAYER_STATE\",\"sender\":\"2\",\"x\":\"nan\",\"y\":\"20\",\"width\":\"40\","
        "\"height\":\"40\",\"velocityX\":\"0\",\"velocityY\":\"0\"}",  // not a finite position
        "{\"type\":\"PLAYER_STATE\",\"sender\":\"2\",\"x\":\"10\",\"y\":\"20\",\"width\":\"-40\","
        "\"height\":\"40\",\"velocityX\":\"0\",\"velocityY\":\"0\"}",  // negative size
    };

    for (const char* input : malformed) {
        INFO(input);
        std::optional<Message> decoded;
        REQUIRE_NOTHROW(decoded = decodeMessage(input));
        REQUIRE_FALSE(decoded.has_value());
    }
}
