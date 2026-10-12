#include <chrono>
#include <cstdint>
#include <cstdlib>

#include "init.hpp"
#include "input.hpp"
#include "render.hpp"
#include "scaling.hpp"
#include "tag_peer.hpp"

namespace {

// Saves the current frame as a BMP. Used by NOVA2D_TAG_SCREENSHOT to check rendering by hand.
void saveFrame(SDL_Renderer* renderer, const char* path) {
    int w = 0;
    int h = 0;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (surface == nullptr) return;
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, surface->pixels,
                         surface->pitch);
    SDL_SaveBMP(surface, path);
    SDL_FreeSurface(surface);
}

}  // namespace

int main(int argc, char* argv[]) {
    // If set, save a screenshot of the game after about one second, then quit
    const char* screenshotPath = std::getenv("NOVA2D_TAG_SCREENSHOT");
    int frame = 0;

    TagConfig config;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!init(window, renderer, "Nova2D - Tag", config.windowWidth, config.windowHeight)) {
        return 1;
    }

    TagPeer peer(config);

    using Clock = std::chrono::steady_clock;
    constexpr auto FRAME = std::chrono::nanoseconds(TagPeer::TICK_NS);
    auto lastFrame = Clock::now();
    bool quit = false;

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                updateWindowSize(window, peer.game());
            }
        }

        const auto now = Clock::now();
        const int64_t elapsedNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastFrame).count();
        lastFrame = now;
        peer.tick(Keys::fromKeyboard(), elapsedNs);

        clearScreen(renderer);
        renderEntities(renderer, peer.game());
        if (screenshotPath != nullptr && ++frame == 60) {
            saveFrame(renderer, screenshotPath);
            quit = true;
        }
        presentScreen(renderer);

        // Cap the frame rate at about 60 FPS
        const auto frameTime = Clock::now() - now;
        if (frameTime < FRAME) {
            SDL_Delay(static_cast<Uint32>(
                std::chrono::duration_cast<std::chrono::milliseconds>(FRAME - frameTime).count()));
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
