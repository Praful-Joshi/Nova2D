#ifndef EVENTMANAGER_HPP
#define EVENTMANAGER_HPP

#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

#include "eventhandler.hpp"

class EventManager {
public:
    EventManager() = default;
    ~EventManager() = default;

    void registerHandler(EventType eventType, EventHandler* handler);
    void deregisterHandler(EventType eventType, EventHandler* handler);

    // Handlers for a game's Custom Event, by name
    void registerHandler(const std::string& customEventName, EventHandler* handler);
    void deregisterHandler(const std::string& customEventName, EventHandler* handler);

    // Queues an event; safe to call from handlers and from other threads
    void raise(Event event);

    // Handles queued events in priority order, including any raised while handling them
    void processEvents();

private:
    std::map<EventType, std::list<EventHandler*>> handlers;
    std::map<std::string, std::list<EventHandler*>> customHandlers;
    std::priority_queue<Event, std::vector<Event>, std::less<>> eventQueue;
    unsigned long long nextSequence = 0;
    std::mutex mutex;  // guards eventQueue and nextSequence

    std::optional<Event> takeNext();
};

#endif
