#include "eventmanager.hpp"


void EventManager::registerHandler(EventType eventType, EventHandler* handler) {
    handlers[eventType].push_back(handler);
}

void EventManager::deregisterHandler(EventType eventType, EventHandler* handler) {
    auto& handlerList = handlers[eventType];
    handlerList.remove(handler);
}

void EventManager::raise(Event event) {
    std::lock_guard<std::mutex> lock(mutex);
    eventQueue.push(event);
}

void EventManager::processEvents() {
    std::lock_guard<std::mutex> lock(mutex);
    while (!eventQueue.empty()) {
        Event event = eventQueue.top();
        eventQueue.pop();
        auto& handlerList = handlers[event.type];
        for (auto* handler : handlerList) {
            handler->onEvent(event);
        }
    }
}