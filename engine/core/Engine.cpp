#include "core/Engine.hpp"

#include "core/Logger.hpp"

namespace Nova {

Engine::Engine(const WindowConfig& cfg) : m_window(cfg) { LOG_INFO("Nova2D Engine initialised."); }

void Engine::run() {
    LOG_INFO("Entering main loop.");

    while (m_running && m_window.isOpen()) {
        m_clock.tick();

        pollEvents();

        // Fixed update — physics/networking at locked 60hz
        while (m_clock.shouldFixedUpdate()) {
            onFixedUpdate();
        }

        // Variable update — game logic every frame
        onUpdate(m_clock.deltaTime());

        // Render
        m_window.beginFrame();
        m_window.clear();
        onRender();
        m_window.endFrame();
    }

    LOG_INFO("Main loop exited.");
}

void Engine::pollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            m_window.close();
        }
        onEvent(e);
    }
}

}  // namespace Nova