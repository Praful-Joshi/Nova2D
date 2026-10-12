#include "message.hpp"

#include <array>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cmath>
#include <exception>
#include <sstream>

namespace pt = boost::property_tree;

namespace {

// Wire names, indexed by MessageType
constexpr std::array<const char*, std::variant_size_v<MessageBody>> TYPE_NAMES = {
    "HEARTBEAT",
    "PLAYER_STATE",
    "LEAVING",
    "HOST_STATE",
    "ROUND_START",
    "ROUND_END",
};

constexpr std::array<const char*, 4> PHASE_NAMES = {"LOBBY", "COUNTDOWN", "PLAYING", "RESULTS"};

void writePeerIds(pt::ptree& tree, const char* key, const std::vector<int>& ids) {
    pt::ptree list;
    for (int id : ids) {
        pt::ptree item;
        item.put_value(id);
        list.push_back({"", item});
    }
    tree.add_child(key, list);
}

// Writers: put a body's fields into the message's JSON object
void writeBody(pt::ptree&, const Heartbeat&) {}
void writeBody(pt::ptree&, const Leaving&) {}
void writeBody(pt::ptree& tree, const PlayerState& state) {
    tree.put("x", state.x);
    tree.put("y", state.y);
    tree.put("width", state.width);
    tree.put("height", state.height);
    tree.put("velocityX", state.velocityX);
    tree.put("velocityY", state.velocityY);
    tree.put("active", state.active);
}

void writeBody(pt::ptree& tree, const HostState& state) {
    tree.put("phase", PHASE_NAMES[static_cast<std::size_t>(state.phase)]);
    tree.put("remainingMs", state.remainingMs);
    tree.put("round", state.round);
    writePeerIds(tree, "participants", state.participants);
    pt::ptree platforms;
    for (const PlatformState& platform : state.platforms) {
        pt::ptree item;
        item.put("id", platform.id);
        item.put("x", platform.x);
        item.put("y", platform.y);
        item.put("directionX", platform.directionX);
        item.put("directionY", platform.directionY);
        platforms.push_back({"", item});
    }
    tree.add_child("platforms", platforms);
}
void writeBody(pt::ptree& tree, const RoundStart& start) {
    tree.put("round", start.round);
    writePeerIds(tree, "participants", start.participants);
}
void writeBody(pt::ptree& tree, const RoundEnd& end) { tree.put("round", end.round); }

// Readers: a body from the JSON object, or nothing if a field is missing or ill-typed
std::optional<double> readFinite(const pt::ptree& tree, const char* key) {
    auto value = tree.get_optional<double>(key);
    if (!value || !std::isfinite(*value)) return std::nullopt;
    return *value;
}

std::optional<int> readSize(const pt::ptree& tree, const char* key) {
    auto value = tree.get_optional<int>(key);
    if (!value || *value < 0) return std::nullopt;
    return *value;
}

std::optional<MessageBody> readPlayerState(const pt::ptree& tree) {
    auto x = readFinite(tree, "x");
    auto y = readFinite(tree, "y");
    auto width = readSize(tree, "width");
    auto height = readSize(tree, "height");
    auto velocityX = readFinite(tree, "velocityX");
    auto velocityY = readFinite(tree, "velocityY");
    auto active = tree.get_optional<bool>("active");
    if (!x || !y || !width || !height || !velocityX || !velocityY || !active) return std::nullopt;
    return PlayerState{*x, *y, *width, *height, *velocityX, *velocityY, *active};
}

std::optional<int> readPeerId(const pt::ptree& node) {
    auto id = node.get_value_optional<int>();
    if (!id || *id < 1 || !node.empty()) return std::nullopt;
    return *id;
}

// A JSON array of Peer IDs (property_tree writes an empty array as "")
std::optional<std::vector<int>> readPeerIds(const pt::ptree& tree, const char* key) {
    auto list = tree.get_child_optional(key);
    if (!list) return std::nullopt;
    std::vector<int> ids;
    for (const auto& [name, item] : *list) {
        auto id = readPeerId(item);
        if (!name.empty() || !id) return std::nullopt;
        ids.push_back(*id);
    }
    return ids;
}

std::optional<RoundPhase> readPhase(const pt::ptree& tree) {
    auto name = tree.get_optional<std::string>("phase");
    if (!name) return std::nullopt;
    for (std::size_t i = 0; i < PHASE_NAMES.size(); ++i) {
        if (*name == PHASE_NAMES[i]) return static_cast<RoundPhase>(i);
    }
    return std::nullopt;
}

std::optional<PlatformState> readPlatform(const pt::ptree& tree) {
    auto id = tree.get_optional<int>("id");
    auto x = readFinite(tree, "x");
    auto y = readFinite(tree, "y");
    auto directionX = tree.get_optional<int>("directionX");
    auto directionY = tree.get_optional<int>("directionY");
    if (!id || !x || !y || !directionX || !directionY) return std::nullopt;
    return PlatformState{*id, *x, *y, *directionX, *directionY};
}

std::optional<MessageBody> readHostState(const pt::ptree& tree) {
    HostState state;
    auto phase = readPhase(tree);
    auto remainingMs = tree.get_optional<int64_t>("remainingMs");
    auto round = tree.get_optional<int>("round");
    auto participants = readPeerIds(tree, "participants");
    auto platforms = tree.get_child_optional("platforms");
    if (!phase || !remainingMs || !round || !participants || !platforms) return std::nullopt;
    state.phase = *phase;
    state.remainingMs = *remainingMs;
    state.round = *round;
    state.participants = *participants;
    for (const auto& [name, item] : *platforms) {
        auto platform = readPlatform(item);
        if (!name.empty() || !platform) return std::nullopt;
        state.platforms.push_back(*platform);
    }
    return state;
}

std::optional<MessageBody> readRoundStart(const pt::ptree& tree) {
    auto round = tree.get_optional<int>("round");
    auto participants = readPeerIds(tree, "participants");
    if (!round || !participants) return std::nullopt;
    return RoundStart{*round, *participants};
}

std::optional<MessageBody> readRoundEnd(const pt::ptree& tree) {
    auto round = tree.get_optional<int>("round");
    if (!round) return std::nullopt;
    return RoundEnd{*round};
}

std::optional<MessageBody> readBody(MessageType type, const pt::ptree& tree) {
    switch (type) {
        case MessageType::HEARTBEAT:
            return Heartbeat{};
        case MessageType::PLAYER_STATE:
            return readPlayerState(tree);
        case MessageType::LEAVING:
            return Leaving{};
        case MessageType::HOST_STATE:
            return readHostState(tree);
        case MessageType::ROUND_START:
            return readRoundStart(tree);
        case MessageType::ROUND_END:
            return readRoundEnd(tree);
    }
    return std::nullopt;
}

}  // namespace

std::string encodeMessage(const Message& message) {
    pt::ptree tree;
    tree.put("type", TYPE_NAMES[message.body.index()]);
    tree.put("sender", message.sender);
    std::visit([&tree](const auto& body) { writeBody(tree, body); }, message.body);

    std::ostringstream json;
    pt::write_json(json, tree, false);
    return json.str();
}

std::optional<Message> decodeMessage(const std::string& json) {
    try {
        pt::ptree tree;
        std::istringstream input(json);
        pt::read_json(input, tree);

        const auto typeNode = tree.get_child_optional("type");
        const auto sender = tree.get_optional<int>("sender");
        if (!typeNode || !typeNode->empty() || !sender || *sender < 1) return std::nullopt;

        for (std::size_t i = 0; i < TYPE_NAMES.size(); ++i) {
            if (typeNode->data() != TYPE_NAMES[i]) continue;
            auto body = readBody(static_cast<MessageType>(i), tree);
            if (!body) return std::nullopt;
            return Message{*sender, *body};
        }
        return std::nullopt;  // unknown type
    } catch (const std::exception&) {
        return std::nullopt;  // not JSON
    }
}
