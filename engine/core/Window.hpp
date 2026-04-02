#pragma once
#include <SDL.h>

#include <string>

#include "math/Vec2.hpp"

namespace Nova {

struct WindowConfig {
    std::string title = "Nova2D";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool resizable = true;
};

class Window {
public:
    // Takes window config and creates an SDL window with the config.
    explicit Window(const WindowConfig& cfg);
    ~Window();

    // Non-copyable, non-movable — owns SDL resources
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void beginFrame();
    void endFrame();  // SDL_RenderPresent + vsync
    void clear(uint8_t r = 20, uint8_t g = 20, uint8_t b = 30);

    SDL_Renderer* renderer() const { return m_renderer; }
    SDL_Window* sdlWindow() const { return m_window; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    Vec2 size() const { return {static_cast<float>(m_width), static_cast<float>(m_height)}; }
    bool isOpen() const { return m_open; }
    void close() { m_open = false; }

private:
    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
    int m_width;
    int m_height;
    bool m_open{true};
};

}  // namespace Nova