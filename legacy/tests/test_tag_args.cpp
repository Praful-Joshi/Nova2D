#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "tag_args.hpp"

namespace {

TagArgs parse(std::vector<const char*> args) {
    args.insert(args.begin(), "nova2d_tag");
    return parseTagArgs(static_cast<int>(args.size()), args.data());
}

}  // namespace

TEST_CASE("With no arguments, Tag runs as Peer 1 of 1", "[legacy][args]") {
    const TagArgs args = parse({});

    REQUIRE(args.error.empty());
    REQUIRE(args.config.peerId == 1);
    REQUIRE(args.config.players == 1);
    REQUIRE(args.config.peerAddress(2) == "localhost");
}

TEST_CASE("The command line sets the Peer ID, player count and other Peers' addresses",
          "[legacy][args]") {
    const TagArgs args =
        parse({"--id", "2", "--players", "3", "--peer", "1=192.168.1.10", "--peer", "3=lan-pc"});

    REQUIRE(args.error.empty());
    REQUIRE(args.config.peerId == 2);
    REQUIRE(args.config.players == 3);
    REQUIRE(args.config.peerAddress(1) == "192.168.1.10");
    REQUIRE(args.config.peerAddress(3) == "lan-pc");
    REQUIRE(args.config.peerAddress(4) == "localhost");
}

TEST_CASE("Invalid command lines are rejected with a reason", "[legacy][args]") {
    const std::vector<std::vector<const char*>> invalid = {
        {"--id", "0"},       {"--id", "5"},          {"--id", "two"},     {"--id"},
        {"--players", "0"},  {"--players", "5"},     {"--peer", "1"},     {"--peer", "9=host"},
        {"--peer", "1="},    {"--peer", "x=host"},   {"--colour", "red"}, {"--id", "2x"},
    };

    for (const auto& args : invalid) {
        INFO(args.front() << (args.size() > 1 ? std::string(" ") + args.back() : ""));
        REQUIRE_FALSE(parse(args).error.empty());
    }
}

TEST_CASE("--help asks for the usage text", "[legacy][args]") {
    const TagArgs args = parse({"--help"});

    REQUIRE(args.helpRequested);
    REQUIRE(tagUsage().find("--peer") != std::string::npos);
}
