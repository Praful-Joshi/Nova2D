#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "eventmanager.hpp"
#include "tag_peer.hpp"

namespace {

// Records the events it receives, by name (Custom Events) or by priority
class Recorder : public EventHandler {
public:
    void onEvent(const Event& event) override {
        names.push_back(event.name());
        priorities.push_back(event.priority);
        numbers.push_back(event.getInt("number"));
    }

    std::vector<std::string> names;
    std::vector<int> priorities;
    std::vector<int> numbers;
};

// Raises a follow-up Custom Event the first time it handles one
class Chainer : public EventHandler {
public:
    explicit Chainer(EventManager& events) : events(events) {}
    void onEvent(const Event& event) override {
        if (event.name() == "FIRST") events.raise(Event::custom("SECOND", event.context, 1, 0));
    }

private:
    EventManager& events;
};

}  // namespace

TEST_CASE("A game can register a handler for a named Custom Event and receive it",
          "[legacy][events]") {
    TagPeer peer(TagConfig{});
    EventManager& events = *peer.game().getEventManager();
    Recorder scored;
    Recorder other;
    events.registerHandler("SCORED", &scored);
    events.registerHandler("OTHER", &other);

    Event event = Event::custom("SCORED", EventContext{&peer.game(), nullptr}, 1, 0);
    event.set("number", 7);
    events.raise(event);
    peer.tick(Keys{});

    REQUIRE(scored.names == std::vector<std::string>{"SCORED"});
    REQUIRE(scored.numbers == std::vector<int>{7});
    REQUIRE(other.names.empty());
}

TEST_CASE("Events run lowest priority value first, and in the order raised within a priority",
          "[legacy][events]") {
    EventManager events;
    Recorder recorder;
    events.registerHandler("E", &recorder);

    const int raised[][2] = {{3, 1}, {1, 2}, {2, 3}, {1, 4}, {3, 5}, {1, 6}};
    for (const auto& [priority, number] : raised) {
        Event event = Event::custom("E", EventContext{nullptr, nullptr}, priority, 0);
        event.set("number", number);
        events.raise(event);
    }
    events.processEvents();

    REQUIRE(recorder.priorities == std::vector<int>{1, 1, 1, 2, 3, 3});
    REQUIRE(recorder.numbers == std::vector<int>{2, 4, 6, 3, 1, 5});
}

TEST_CASE("A handler can raise an event, which is handled in the same pass", "[legacy][events]") {
    EventManager events;
    Chainer chainer(events);
    Recorder recorder;
    events.registerHandler("FIRST", &chainer);
    events.registerHandler("SECOND", &recorder);

    events.raise(Event::custom("FIRST", EventContext{nullptr, nullptr}, 1, 0));
    events.processEvents();

    REQUIRE(recorder.names == std::vector<std::string>{"SECOND"});
}
