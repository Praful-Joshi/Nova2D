#include "tag_args.hpp"

#include <optional>

#include "arena.hpp"

namespace {

// A whole number from 1 to MAX_PLAYERS
std::optional<int> parsePeerNumber(const std::string& text) {
    if (text.size() != 1 || text[0] < '1' || text[0] > '0' + arena::MAX_PLAYERS) {
        return std::nullopt;
    }
    return text[0] - '0';
}

}  // namespace

TagArgs parseTagArgs(int argc, const char* const argv[]) {
    TagArgs args;
    const std::string range = "1 to " + std::to_string(arena::MAX_PLAYERS);

    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];
        if (flag == "--help" || flag == "-h") {
            args.helpRequested = true;
            continue;
        }
        if (flag != "--id" && flag != "--players" && flag != "--peer") {
            args.error = "unknown option " + flag;
            return args;
        }
        if (i + 1 >= argc) {
            args.error = flag + " needs a value";
            return args;
        }
        const std::string value = argv[++i];

        if (flag == "--peer") {
            // id=address
            const auto equals = value.find('=');
            const auto peerId = parsePeerNumber(value.substr(0, equals));
            if (equals == std::string::npos || !peerId || equals + 1 == value.size()) {
                args.error = "--peer takes id=address, with an id from " + range;
                return args;
            }
            args.config.peerAddresses[*peerId] = value.substr(equals + 1);
            continue;
        }

        const auto number = parsePeerNumber(value);
        if (!number) {
            args.error = flag + " must be from " + range;
            return args;
        }
        if (flag == "--id") {
            args.config.peerId = *number;
        } else {
            args.config.players = *number;
        }
    }
    return args;
}

std::string tagUsage() {
    return "Usage: nova2d_tag [--id N] [--players N] [--peer id=address]...\n"
           "  --id N              this Peer's ID, 1-4 (default 1)\n"
           "  --players N         number of players expected, 1-4 (default 1)\n"
           "  --peer id=address   host name or IP address of another Peer (repeatable);\n"
           "                      Peers without one are on localhost\n"
           "Peer N listens on TCP port 5550 + N.\n";
}
