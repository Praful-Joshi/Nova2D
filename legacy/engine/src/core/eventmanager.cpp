#include "eventmanager.hpp"

void EventManager::registerHandler(EventType eventType, EventHandler* handler) {
    handlers[eventType].push_back(handler);
}

void EventManager::deregisterHandler(EventType eventType, EventHandler* handler) {
    handlers[eventType].remove(handler);
}

void EventManager::registerHandler(const std::string& customEventName, EventHandler* handler) {
    customHandlers[customEventName].push_back(handler);
}

void EventManager::deregisterHandler(const std::string& customEventName, EventHandler* handler) {
    customHandlers[customEventName].remove(handler);
}

void EventManager::raise(Event event) {
    std::lock_guard<std::mutex> lock(mutex);
    event.sequence = nextSequence++;
    eventQueue.push(event);
}

void EventManager::processEvents() {
    // Take one event at a time and handle it without holding the lock, so handlers can raise
    while (std::optional<Event> event = takeNext()) {
        const auto& handlerList = event->type == EventType::CUSTOM
                                      ? customHandlers[event->name()]
                                      : handlers[event->type];
        for (auto* handler : handlerList) {
            handler->onEvent(*event);
        }
    }
}

std::optional<Event> EventManager::takeNext() {
    std::lock_guard<std::mutex> lock(mutex);
    if (eventQueue.empty()) return std::nullopt;
    Event next = eventQueue.top();
    eventQueue.pop();
    return next;
}
