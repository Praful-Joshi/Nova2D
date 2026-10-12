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
};

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
}

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
    if (!x || !y || !width || !height || !velocityX || !velocityY) return std::nullopt;
    return PlayerState{*x, *y, *width, *height, *velocityX, *velocityY};
}

std::optional<MessageBody> readBody(MessageType type, const pt::ptree& tree) {
    switch (type) {
        case MessageType::HEARTBEAT:
            return Heartbeat{};
        case MessageType::PLAYER_STATE:
            return readPlayerState(tree);
        case MessageType::LEAVING:
            return Leaving{};
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
