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

TEST_CASE("A HOST_STATE survives encoding and decoding unchanged", "[legacy][messages]") {
    HostState state;
    state.phase = RoundPhase::PLAYING;
    state.remainingMs = 87'654;
    state.round = 3;
    state.participants = {1, 2, 4};
    state.platforms = {PlatformState{10, 812.25, 590, 1, 1}, PlatformState{12, 1060, 433.5, 1, -1}};
    const Message message{1, state};

    const std::optional<Message> decoded = decodeMessage(encodeMessage(message));

    REQUIRE(decoded.has_value());
    REQUIRE(decoded->type() == MessageType::HOST_STATE);
    REQUIRE(*decoded == message);
}

TEST_CASE("A HOST_STATE with no participants or platforms survives encoding and decoding",
          "[legacy][messages]") {
    const Message message{2, HostState{}};

    const std::optional<Message> decoded = decodeMessage(encodeMessage(message));

    REQUIRE(decoded.has_value());
    REQUIRE(*decoded == message);
}

TEST_CASE("ROUND_START and ROUND_END survive encoding and decoding unchanged",
          "[legacy][messages]") {
    const Message start{1, RoundStart{5, {1, 3}}};
    const Message end{1, RoundEnd{5, {}}};

    REQUIRE(decodeMessage(encodeMessage(start)) == std::optional<Message>(start));
    REQUIRE(decodeMessage(encodeMessage(end)) == std::optional<Message>(end));
    REQUIRE(decodeMessage(encodeMessage(start))->type() == MessageType::ROUND_START);
    REQUIRE(decodeMessage(encodeMessage(end))->type() == MessageType::ROUND_END);
}

TEST_CASE("Malformed Round messages are rejected without throwing", "[legacy][messages]") {
    const char* malformed[] = {
        // unknown phase
        "{\"type\":\"HOST_STATE\",\"sender\":\"1\",\"phase\":\"NAPPING\",\"remainingMs\":\"1\","
        "\"round\":\"1\",\"participants\":\"\",\"platforms\":\"\"}",
        // participant isn't a Peer ID
        "{\"type\":\"HOST_STATE\",\"sender\":\"1\",\"phase\":\"LOBBY\",\"remainingMs\":\"1\","
        "\"round\":\"1\",\"participants\":[\"two\"],\"platforms\":\"\"}",
        // platform without a position
        "{\"type\":\"HOST_STATE\",\"sender\":\"1\",\"phase\":\"LOBBY\",\"remainingMs\":\"1\","
        "\"round\":\"1\",\"participants\":\"\",\"platforms\":[{\"id\":\"10\"}]}",
        // no round
        "{\"type\":\"ROUND_START\",\"sender\":\"1\",\"participants\":[\"1\"]}",
        "{\"type\":\"ROUND_END\",\"sender\":\"1\"}",
    };
    for (const char* input : malformed) {
        INFO(input);
        REQUIRE_FALSE(decodeMessage(input).has_value());
    }
}

TEST_CASE("A HOST_STATE carries who is It, the It freeze, Tag-back Immunity and It times",
          "[legacy][messages][tagging]") {
    HostState state;
    state.phase = RoundPhase::PLAYING;
    state.remainingMs = 60'000;
    state.round = 2;
    state.participants = {1, 2, 3};
    state.it = 3;
    state.itFrozenMs = 1'250;
    state.immune = 1;
    state.immuneMs = 875;
    state.itTimes = {ItTime{1, 41'000}, ItTime{2, 0}, ItTime{3, 19'000}};
    const Message message{1, state};

    const std::optional<Message> decoded = decodeMessage(encodeMessage(message));

    REQUIRE(decoded.has_value());
    REQUIRE(*decoded == message);
}

TEST_CASE("ROUND_START with It, ROUND_END with It times and TAGGED survive encoding and "
          "decoding unchanged",
          "[legacy][messages][tagging]") {
    const Message start{1, RoundStart{4, {1, 2, 4}, 2}};
    const Message end{1, RoundEnd{4, {ItTime{1, 30'000}, ItTime{2, 90'000}}}};
    const Message tagged{2, Tagged{3, 1}};

    REQUIRE(decodeMessage(encodeMessage(start)) == std::optional<Message>(start));
    REQUIRE(decodeMessage(encodeMessage(end)) == std::optional<Message>(end));
    REQUIRE(decodeMessage(encodeMessage(tagged)) == std::optional<Message>(tagged));
    REQUIRE(decodeMessage(encodeMessage(tagged))->type() == MessageType::TAGGED);
    REQUIRE(encodeMessage(tagged).find("\"type\":\"TAGGED\"") != std::string::npos);
}

TEST_CASE("Malformed Tagging messages are rejected without throwing",
          "[legacy][messages][tagging]") {
    const char* malformed[] = {
        "{\"type\":\"TAGGED\",\"sender\":\"1\",\"from\":\"2\"}",               // no to
        "{\"type\":\"TAGGED\",\"sender\":\"1\",\"from\":\"0\",\"to\":\"2\"}",  // not a Peer ID
        "{\"type\":\"TAGGED\",\"sender\":\"1\",\"from\":\"x\",\"to\":\"2\"}",  // ill-typed
        // an It time without a player
        "{\"type\":\"ROUND_END\",\"sender\":\"1\",\"round\":\"1\",\"itTimes\":[{\"ms\":\"5\"}]}",
        // a negative It time
        "{\"type\":\"ROUND_END\",\"sender\":\"1\",\"round\":\"1\","
        "\"itTimes\":[{\"peerId\":\"1\",\"ms\":\"-5\"}]}",
        // no It times
        "{\"type\":\"ROUND_END\",\"sender\":\"1\",\"round\":\"1\"}",
        // It isn't a Peer ID
        "{\"type\":\"ROUND_START\",\"sender\":\"1\",\"round\":\"1\",\"participants\":[\"1\"],"
        "\"it\":\"-1\"}",
        // no It
        "{\"type\":\"HOST_STATE\",\"sender\":\"1\",\"phase\":\"LOBBY\",\"remainingMs\":\"1\","
        "\"round\":\"1\",\"participants\":\"\",\"platforms\":\"\",\"itFrozenMs\":\"0\","
        "\"immune\":\"0\",\"immuneMs\":\"0\",\"itTimes\":\"\"}",
    };
    for (const char* input : malformed) {
        INFO(input);
        std::optional<Message> decoded;
        REQUIRE_NOTHROW(decoded = decodeMessage(input));
        REQUIRE_FALSE(decoded.has_value());
    }
}
