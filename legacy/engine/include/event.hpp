#ifndef EVENT_HPP
#define EVENT_HPP

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>

class Entity;  // Forward declaration to avoid circular dependencies
class Game;    // Forward declaration for the Game class

// CUSTOM is a game-defined event, identified by its name rather than by this enum
enum class EventType { COLLISION, DEATH, SPAWN, CONTROL, SIDEBOUNDARY, CUSTOM };

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
    int priority;                  // Lower values are handled first
    unsigned long long timestamp;  // From Timeline; older events are handled first
    unsigned long long sequence = 0;  // Set by EventManager::raise; earlier raised first

    // Order in the EventManager's queue: the queue handles the "greatest" event first, so an
    // event is "less" than another when it should be handled after it
    bool operator<(const Event& other) const {
        if (priority != other.priority) return priority > other.priority;
        if (timestamp != other.timestamp) return timestamp > other.timestamp;
        return sequence > other.sequence;
    }

    Event(EventType type, EventContext context, int priority, unsigned long long timestamp)
        : type(type), context(context), priority(priority), timestamp(timestamp) {}

    // A Custom Event with the given name
    static Event custom(const std::string& name, EventContext context, int priority,
                        unsigned long long timestamp) {
        Event event(EventType::CUSTOM, context, priority, timestamp);
        event.set("name", name);
        return event;
    }

    // The Custom Event's name, or empty for engine events
    [[nodiscard]] std::string name() const { return getString("name"); }

    void set(const std::string& key, int value) {
        Variant v;
        v.type = Variant::Type::INT;
        v.asInt = value;
        parameters[key] = v;
    }
    void set(const std::string& key, bool value) {
        Variant v;
        v.type = Variant::Type::BOOL;
        v.asBool = value;
        parameters[key] = v;
    }
    void set(const std::string& key, const std::string& value) {
        Variant v;
        v.type = Variant::Type::STRING;
        v.asInt = 0;
        v.asString = value;
        parameters[key] = v;
    }
    void set(const std::string& key, const char* value) { set(key, std::string(value)); }

    // Parameter values, or the fallback if missing or of another type
    [[nodiscard]] int getInt(const std::string& key, int fallback = 0) const {
        auto it = parameters.find(key);
        return it != parameters.end() && it->second.type == Variant::Type::INT ? it->second.asInt
                                                                                : fallback;
    }
    [[nodiscard]] bool getBool(const std::string& key, bool fallback = false) const {
        auto it = parameters.find(key);
        return it != parameters.end() && it->second.type == Variant::Type::BOOL
                   ? it->second.asBool
                   : fallback;
    }
    [[nodiscard]] std::string getString(const std::string& key) const {
        auto it = parameters.find(key);
        return it != parameters.end() && it->second.type == Variant::Type::STRING
                   ? it->second.asString
                   : std::string();
    }
};

#endif
