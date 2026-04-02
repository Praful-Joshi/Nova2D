#ifndef EVENTHANDLER_HPP
#define EVENTHANDLER_HPP

#include "event.hpp"

class EventHandler {
public:
    virtual void onEvent(const Event& event) = 0;
    virtual ~EventHandler() = default;
};

#endif