#ifndef EVENT_HPP
#define EVENT_HPP

#include <map>
#include <string>
#include <unordered_map>
#include <variant>

class Entity;  // Forward declaration to avoid circular dependencies
class Game;    // Forward declaration for the Game class

enum class EventType { COLLISION, DEATH, SPAWN, CONTROL, SIDEBOUNDARY };

struct EventContext {
    Game* game;      // Reference to the game object
    Entity* entity;  // Reference to the entity involved in the event
};

struct Variant {
    enum class Type { INT, FLOAT, STRING, BOOL, ENTITY_PTR };
    Type type;

    union {
        int asInt;
        float asFloat;
        bool asBool;
        void* asEntityPtr;
    };
    std::string asString;
};

class Event {
public:
    EventType type;
    EventContext context{nullptr, nullptr};

    std::unordered_map<std::string, Variant> parameters;
    int priority;                  // Priority level for event queue
    unsigned long long timestamp;  // From Timeline

    bool operator<(const Event& other) const {
        // Priority first
        if (priority == other.priority) {
            // If priority equal, choose older event
            return timestamp > other.timestamp;
        }
        return priority > other.priority;  // Lower priority first
    }

    Event(EventType type, EventContext context, int priority, unsigned long long timestamp)
        : type(type), context(context), priority(priority), timestamp(timestamp) {}
};

#endif
