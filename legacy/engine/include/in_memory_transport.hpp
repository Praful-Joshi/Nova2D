#ifndef IN_MEMORY_TRANSPORT_HPP
#define IN_MEMORY_TRANSPORT_HPP

#include <mutex>
#include <string>
#include <vector>

#include "transport.hpp"

class InMemoryTransport;

// An in-process stand-in for the network, so tests can connect several headless Peers.
// Deterministic: a message sent now is in every other connected transport's next receive().
// It must outlive the transports connected to it.
class InMemoryNetwork {
public:
    InMemoryNetwork() = default;
    InMemoryNetwork(const InMemoryNetwork&) = delete;
    InMemoryNetwork& operator=(const InMemoryNetwork&) = delete;

private:
    friend class InMemoryTransport;

    std::mutex mutex;
    std::vector<InMemoryTransport*> members;

    void join(InMemoryTransport* member);
    void leave(InMemoryTransport* member);
    void deliver(const InMemoryTransport* sender, const std::string& message);
    std::vector<std::string> collect(InMemoryTransport* member);
};

// One Peer's connection to an InMemoryNetwork, from construction until destruction
// (or disconnect(), which simulates the Peer's network going away)
class InMemoryTransport : public Transport {
public:
    explicit InMemoryTransport(InMemoryNetwork& network);
    ~InMemoryTransport() override;
    InMemoryTransport(const InMemoryTransport&) = delete;
    InMemoryTransport& operator=(const InMemoryTransport&) = delete;

    void send(const std::string& message) override;
    std::vector<std::string> receive() override;

    // Stops sending and receiving, as if the Peer had crashed
    void disconnect();

private:
    friend class InMemoryNetwork;

    InMemoryNetwork* network;  // nullptr once disconnected
    std::vector<std::string> inbox;  // guarded by the network's mutex
};

#endif
