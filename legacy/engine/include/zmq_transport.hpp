#ifndef ZMQ_TRANSPORT_HPP
#define ZMQ_TRANSPORT_HPP

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <zmq.hpp>

#include "transport.hpp"

// The full-mesh ZeroMQ transport (ADR-0001). This Peer binds one PUB socket on
// tcp://*:<5550 + Peer ID>, and its SUB socket connects to every other Peer's PUB socket.
// ZeroMQ reconnects on its own, so Peers can start in any order.
//
// Threading: a receive thread owns the SUB socket and only pushes raw messages into a
// mutex-guarded inbox; send() and receive() run on the game thread, which owns the PUB socket.
class ZmqTransport : public Transport {
public:
    static constexpr int BASE_PORT = 5550;

    // otherPeers maps each other Peer's ID to its host name or IP address.
    // Throws zmq::error_t if this Peer's port can't be bound.
    ZmqTransport(int peerId, const std::map<int, std::string>& otherPeers);
    ~ZmqTransport() override;
    ZmqTransport(const ZmqTransport&) = delete;
    ZmqTransport& operator=(const ZmqTransport&) = delete;

    void send(const std::string& message) override;
    std::vector<std::string> receive() override;

    static int port(int peerId) { return BASE_PORT + peerId; }

private:
    zmq::context_t context;
    zmq::socket_t publisher;
    zmq::socket_t subscriber;

    std::mutex inboxMutex;
    std::vector<std::string> inbox;
    std::atomic<bool> stopping{false};
    std::thread receiver;

    void receiveLoop();
};

#endif
