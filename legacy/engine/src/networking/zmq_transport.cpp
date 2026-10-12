#include "zmq_transport.hpp"

#include <utility>

namespace {
// How long the receive thread waits for a message before checking whether to stop
constexpr int RECEIVE_TIMEOUT_MS = 50;
// How long closing waits to flush unsent messages, such as LEAVING
constexpr int LINGER_MS = 500;
}  // namespace

ZmqTransport::ZmqTransport(int peerId, const std::map<int, std::string>& otherPeers)
    : context(1),
      publisher(context, zmq::socket_type::pub),
      subscriber(context, zmq::socket_type::sub) {
    publisher.set(zmq::sockopt::linger, LINGER_MS);
    publisher.bind("tcp://*:" + std::to_string(port(peerId)));

    subscriber.set(zmq::sockopt::linger, 0);
    subscriber.set(zmq::sockopt::rcvtimeo, RECEIVE_TIMEOUT_MS);
    subscriber.set(zmq::sockopt::subscribe, "");
    for (const auto& [otherId, address] : otherPeers) {
        if (otherId == peerId) continue;
        subscriber.connect("tcp://" + address + ":" + std::to_string(port(otherId)));
    }

    receiver = std::thread([this] { receiveLoop(); });
}

ZmqTransport::~ZmqTransport() {
    stopping = true;
    receiver.join();
}

void ZmqTransport::send(const std::string& message) {
    // PUB sockets never block; with no subscribers the message is dropped
    publisher.send(zmq::buffer(message), zmq::send_flags::dontwait);
}

std::vector<std::string> ZmqTransport::receive() {
    std::lock_guard<std::mutex> lock(inboxMutex);
    return std::exchange(inbox, {});
}

void ZmqTransport::receiveLoop() {
    while (!stopping) {
        zmq::message_t message;
        try {
            if (!subscriber.recv(message)) continue;  // timed out
        } catch (const zmq::error_t&) {
            return;
        }
        std::lock_guard<std::mutex> lock(inboxMutex);
        inbox.push_back(message.to_string());
    }
}
