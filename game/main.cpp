#include <SDL.h>

#include "core/Engine.hpp"
#include "core/Logger.hpp"

// A minimal game that inherits Engine and renders a bouncing rectangle.
// This is the Phase 1 milestone — proves the loop, clock, and renderer work.
class TestGame : public Nova::Engine {
public:
    TestGame() : Nova::Engine({"Nova2D - Phase 1", 1280, 720}) {}

protected:
    void onFixedUpdate() override {
        // Move at fixed 60hz — physics-safe
        m_pos += m_vel * Nova::Clock::FIXED_DT;

        // Bounce off window edges
        if (m_pos.x < 0 || m_pos.x + SIZE > window().width()) m_vel.x *= -1.f;
        if (m_pos.y < 0 || m_pos.y + SIZE > window().height()) m_vel.y *= -1.f;
    }

    void onRender() override {
        // Interpolate position between last two physics states for smooth rendering
        float alpha = clock().alpha();
        Nova::Vec2 renderPos = m_pos + m_vel * Nova::Clock::FIXED_DT * alpha;

        SDL_Rect rect{static_cast<int>(renderPos.x), static_cast<int>(renderPos.y), SIZE, SIZE};
        SDL_SetRenderDrawColor(window().renderer(), 80, 180, 255, 255);
        SDL_RenderFillRect(window().renderer(), &rect);
    }

    void onEvent(SDL_Event& e) override {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) window().close();
    }

private:
    static constexpr int SIZE = 80;
    Nova::Vec2 m_pos{100.f, 100.f};
    Nova::Vec2 m_vel{220.f, 180.f};  // pixels per second
};

int main(int argc, char* argv[]) {
    Nova::initLogger();
    LOG_INFO("Nova2D Phase 1 starting.");

    try {
        TestGame game;
        game.run();
    } catch (const std::exception& e) {
        LOG_CRITICAL("Fatal: {}", e.what());
        return 1;
    }

    return 0;
}