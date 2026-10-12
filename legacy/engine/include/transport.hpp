#ifndef TRANSPORT_HPP
#define TRANSPORT_HPP

#include <string>
#include <vector>

// How a Peer exchanges messages with every other Peer (a full mesh, see ADR-0001).
// One message is one string. Delivery is best effort: a Peer that isn't listening yet
// misses what was sent before it arrived.
//
// Threading: send() and receive() are called from the game thread only. An implementation
// may receive on its own thread, but then hands messages over through a locked inbox.
class Transport {
public:
    virtual ~Transport() = default;

    // Sends one message to all other Peers
    virtual void send(const std::string& message) = 0;

    // The messages that arrived since the last call, oldest first
    virtual std::vector<std::string> receive() = 0;
};

#endif
