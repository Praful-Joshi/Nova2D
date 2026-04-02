#include "core/Window.hpp"

#include <stdexcept>

#include "core/Logger.hpp"

namespace Nova {

Window::Window(const WindowConfig& cfg) : m_width(cfg.width), m_height(cfg.height) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

    uint32_t flags = SDL_WINDOW_SHOWN;
    if (cfg.resizable) flags |= SDL_WINDOW_RESIZABLE;

    m_window = SDL_CreateWindow(cfg.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                cfg.width, cfg.height, flags);
    if (!m_window)
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

    uint32_t rendererFlags = SDL_RENDERER_ACCELERATED;
    if (cfg.vsync) rendererFlags |= SDL_RENDERER_PRESENTVSYNC;

    m_renderer = SDL_CreateRenderer(m_window, -1, rendererFlags);
    if (!m_renderer)
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

    LOG_INFO("Window created: {}x{} '{}'", cfg.width, cfg.height, cfg.title);
}

Window::~Window() {
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
    LOG_INFO("Window destroyed.");
}

void Window::clear(uint8_t r, uint8_t g, uint8_t b) {
    SDL_SetRenderDrawColor(m_renderer, r, g, b, 255);
    SDL_RenderClear(m_renderer);
}

void Window::beginFrame() {}

void Window::endFrame() { SDL_RenderPresent(m_renderer); }

}  // namespace Nova