#include "tag_network.hpp"

#include <utility>

#include "arena.hpp"

namespace {

// Schedules the next send after one that was due at `next`, without bunching up after a stall
int64_t nextDue(int64_t next, int64_t interval, int64_t nowNs) {
    next += interval;
    return next > nowNs ? next : nowNs + interval;
}

double lerp(double from, double to, double t) { return from + (to - from) * t; }

void place(Entity& player, const PlayerState& state) {
    player.setPosition(state.x, state.y);
    player.setRectWidth(state.width);
    player.setRectHeight(state.height);
    player.setVelocityX(state.velocityX);
    player.setVelocityY(state.velocityY);
    player.setActive(state.active);
}

}  // namespace

TagNetwork::TagNetwork(int peerId, std::shared_ptr<Transport> transport)
    : peerId_(peerId), transport_(std::move(transport)) {}

std::vector<Message> TagNetwork::receive(Game& game, int64_t nowNs) {
    departed_.clear();
    std::vector<Message> messages;
    if (!transport_) return messages;

    for (const std::string& raw : transport_->receive()) {
        std::optional<Message> message = decodeMessage(raw);
        if (!message) continue;
        const int sender = message->sender;
        if (sender == peerId_ || sender > arena::MAX_PLAYERS) continue;

        if (message->type() == MessageType::LEAVING) {
            if (remotes_.count(sender) != 0) depart(game, sender);
        } else {
            // Any other message shows the sender is here; the first one adds its player
            auto [remote, isNew] = remotes_.try_emplace(sender);
            if (isNew) {
                arena::addPlayer(game, sender, false);
                forceFullState();  // it hasn't seen state that hasn't changed lately
            }
            remote->second.lastHeardNs = nowNs;
            if (const auto* state = std::get_if<PlayerState>(&message->body)) {
                auto& snapshots = remote->second.snapshots;
                // After a quiet spell (a still player sends nothing under change-only
                // syncing), the player held its last state until just before this one: without
                // that, it would be drawn moving towards this state across the whole spell
                if (!snapshots.empty() &&
                    nowNs - snapshots.back().receivedNs > 2 * STATE_INTERVAL_NS) {
                    snapshots.push_back(
                        Snapshot{nowNs - STATE_INTERVAL_NS, snapshots.back().state});
                }
                snapshots.push_back(Snapshot{nowNs, *state});
            }
        }
        messages.push_back(std::move(*message));
    }

    std::vector<int> silent;
    for (const auto& [remoteId, remote] : remotes_) {
        if (nowNs - remote.lastHeardNs >= TIMEOUT_NS) silent.push_back(remoteId);
    }
    for (int remoteId : silent) depart(game, remoteId);
    return messages;
}

void TagNetwork::interpolate(Game& game, int64_t nowNs) {
    const int64_t renderNs = nowNs - INTERPOLATION_DELAY_NS;
    for (auto& [remoteId, remote] : remotes_) {
        auto& snapshots = remote.snapshots;
        // Keep the latest snapshot at or before the render time, and everything after it
        while (snapshots.size() >= 2 && snapshots[1].receivedNs <= renderNs) {
            snapshots.pop_front();
        }
        auto player = game.getEntity(arena::playerEntityId(remoteId));
        if (snapshots.empty() || player == nullptr) continue;

        const Snapshot& from = snapshots.front();
        if (snapshots.size() == 1 || renderNs <= from.receivedNs) {
            place(*player, from.state);  // nothing to interpolate towards yet
            continue;
        }
        const Snapshot& to = snapshots[1];
        if (!from.state.active || !to.state.active) {
            // Hiding or respawning is a jump, not movement: don't slide across the arena
            place(*player, from.state);
            continue;
        }
        const double t = static_cast<double>(renderNs - from.receivedNs) /
                         static_cast<double>(to.receivedNs - from.receivedNs);
        PlayerState between = from.state;
        between.x = lerp(from.state.x, to.state.x, t);
        between.y = lerp(from.state.y, to.state.y, t);
        place(*player, between);
    }
}

void TagNetwork::sendDue(const Entity* localPlayer, int64_t nowNs) {
    if (!transport_) return;
    if (localPlayer != nullptr && nowNs >= nextStateNs_) {
        sendState(PlayerState{localPlayer->getX(), localPlayer->getY(), localPlayer->getRect().w,
                              localPlayer->getRect().h, localPlayer->getVelocityX(),
                              localPlayer->getVelocityY(), localPlayer->isActive()});
        nextStateNs_ = nextDue(nextStateNs_, STATE_INTERVAL_NS, nowNs);
    }
    if (nowNs >= nextHeartbeatNs_) {
        send(Heartbeat{});
        nextHeartbeatNs_ = nextDue(nextHeartbeatNs_, HEARTBEAT_INTERVAL_NS, nowNs);
    }
}

void TagNetwork::send(const MessageBody& body) {
    if (transport_) transport_->send(encodeMessage(Message{peerId_, body}));
}

void TagNetwork::sendState(const MessageBody& body) {
    const auto type = static_cast<MessageType>(body.index());
    auto last = lastSent_.find(type);
    if (changeOnly_ && last != lastSent_.end() && last->second == body) return;
    send(body);
    lastSent_.insert_or_assign(type, body);
}

void TagNetwork::leave() {
    send(Leaving{});
    transport_.reset();
}

std::vector<int> TagNetwork::connectedPeers() const {
    std::vector<int> ids;
    for (const auto& [remoteId, remote] : remotes_) ids.push_back(remoteId);
    return ids;
}

void TagNetwork::depart(Game& game, int peerId) {
    if (auto player = game.getEntity(arena::playerEntityId(peerId))) game.removeEntity(player);
    remotes_.erase(peerId);
    departed_.push_back(peerId);
}
