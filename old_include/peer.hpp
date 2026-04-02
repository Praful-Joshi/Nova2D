#ifndef PEER_HPP
#define PEER_HPP

#include <string>
#include <zmq.hpp>

class Peer {
public:
    Peer(bool isServer, int clientID);
    void send_data(const std::vector<std::string> message);
    std::string receive_data();

private:
    zmq::context_t context;
    zmq::socket_t publisher;
    zmq::socket_t subscriber;
};

#endif  // PEER_HPP
