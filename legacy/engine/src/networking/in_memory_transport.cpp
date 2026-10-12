#include "in_memory_transport.hpp"

#include <algorithm>
#include <utility>

void InMemoryNetwork::join(InMemoryTransport* member) {
    std::lock_guard<std::mutex> lock(mutex);
    members.push_back(member);
}

void InMemoryNetwork::leave(InMemoryTransport* member) {
    std::lock_guard<std::mutex> lock(mutex);
    members.erase(std::remove(members.begin(), members.end(), member), members.end());
}

void InMemoryNetwork::deliver(const InMemoryTransport* sender, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);
    for (InMemoryTransport* member : members) {
        if (member != sender) member->inbox.push_back(message);
    }
}

std::vector<std::string> InMemoryNetwork::collect(InMemoryTransport* member) {
    std::lock_guard<std::mutex> lock(mutex);
    return std::exchange(member->inbox, {});
}

InMemoryTransport::InMemoryTransport(InMemoryNetwork& network) : network(&network) {
    network.join(this);
}

InMemoryTransport::~InMemoryTransport() { disconnect(); }

void InMemoryTransport::send(const std::string& message) {
    if (network != nullptr) network->deliver(this, message);
}

std::vector<std::string> InMemoryTransport::receive() {
    if (network == nullptr) return {};
    return network->collect(this);
}

void InMemoryTransport::disconnect() {
    if (network == nullptr) return;
    network->leave(this);
    network = nullptr;
}
