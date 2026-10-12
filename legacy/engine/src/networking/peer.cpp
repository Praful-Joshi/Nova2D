#include "peer.hpp"

#include <iostream>

Peer::Peer(bool isServer, int clientID)
    : context(15),
      publisher(context, zmq::socket_type::pub),
      subscriber(context, zmq::socket_type::sub) {
    if (isServer) {
        publisher.bind("172.26.58.166");
        subscriber.connect("172.26.58.167");
        subscriber.connect("172.26.58.168");
        subscriber.connect("tcp://localhost:5558");
        subscriber.connect("tcp://localhost:5559");
        subscriber.connect("tcp://localhost:5560");
        subscriber.connect("tcp://localhost:5561");
        subscriber.connect("tcp://localhost:5562");
        subscriber.connect("tcp://localhost:5563");
        subscriber.connect("tcp://localhost:5564");
        // Set the socket option to subscribe
        subscriber.set(zmq::sockopt::subscribe, "");
    } else {
        switch (clientID) {
            case 2:
                publisher.bind("172.26.58.167");
                break;
            case 3:
                publisher.bind("172.26.58.168");
                break;
            case 4:
                publisher.bind("tcp://localhost:5558");
                break;
            case 5:
                publisher.bind("tcp://localhost:5559");
                break;
            case 6:
                publisher.bind("tcp://localhost:5560");
                break;
            case 7:
                publisher.bind("tcp://localhost:5561");
                break;
            case 8:
                publisher.bind("tcp://localhost:5562");
                break;
            case 9:
                publisher.bind("tcp://localhost:5563");
                break;
            case 10:
                publisher.bind("tcp://localhost:5564");
                break;
            default:
                publisher.bind("tcp://localhost:5556");
                break;
        }
        subscriber.connect("tcp://localhost:5555");
        // Set the socket option to subscribe
        subscriber.set(zmq::sockopt::subscribe, "");
    }
}

void Peer::send_data(std::vector<std::string> data) {
    for (size_t i = 0; i < data.size(); ++i) {
        zmq::message_t message(data[i].length());
        memcpy(message.data(), data[i].c_str(), data[i].length());

        // Check if it's the last message part; send it without the 'sndmore' flag
        if (i == data.size() - 1) {
            publisher.send(message, zmq::send_flags::none);
        } else {
            publisher.send(message, zmq::send_flags::sndmore);
        }
    }
}

std::string Peer::receive_data() {
    zmq::pollitem_t items[] = {{subscriber, 0, ZMQ_POLLIN, 0}};
    zmq::poll(&items[0], 1, std::chrono::milliseconds(1000));  // Wait for up to 1 second

    if (items[0].revents & ZMQ_POLLIN) {
        zmq::message_t recvMessage;
        [[maybe_unused]] auto result = subscriber.recv(recvMessage, zmq::recv_flags::none);
        std::string receivedData(static_cast<char*>(recvMessage.data()), recvMessage.size());
        return receivedData;
    } else {
        return "";
    }
}