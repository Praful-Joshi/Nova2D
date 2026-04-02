#include "core/Clock.hpp"

#include <SDL.h>

namespace Nova {

Clock::Clock() { m_lastTick = SDL_GetPerformanceCounter(); }

float Clock::tick() {
    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t freq = SDL_GetPerformanceFrequency();
    m_deltaTime = static_cast<float>(now - m_lastTick) / static_cast<float>(freq);
    m_lastTick = now;

    // Clamp to 250ms max — prevents "spiral of death" if the app freezes
    // (e.g. dragging the window) and accumulates a huge backlog of physics steps
    if (m_deltaTime > 0.25f) m_deltaTime = 0.25f;

    m_accumulator += m_deltaTime;
    m_totalTime += m_deltaTime;
    ++m_frameCount;

    return m_deltaTime;
}

bool Clock::shouldFixedUpdate() {
    if (m_accumulator >= FIXED_DT) {
        m_accumulator -= FIXED_DT;
        return true;
    }
    return false;
}

}  // namespace Nova