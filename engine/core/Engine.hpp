#pragma once
#include "core/Clock.hpp"
#include "core/Window.hpp"

namespace Nova {

class Engine {
public:
    explicit Engine(const WindowConfig& cfg);
    ~Engine() = default;

    // Non-copyable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Run the main loop. Returns when window is closed.
    void run();

    Window& window() { return m_window; }
    Clock& clock() { return m_clock; }

protected:
    // Override these in your game
    virtual void onUpdate(float dt) {}     // called every frame
    virtual void onFixedUpdate() {}        // called at fixed 60hz
    virtual void onRender() {}             // called every frame after update
    virtual void onEvent(SDL_Event& e) {}  // called per SDL event

private:
    void pollEvents();

    Window m_window;
    Clock m_clock;
    bool m_running{true};
};

}  // namespace Nova