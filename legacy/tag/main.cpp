#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "arena.hpp"
#include "hud.hpp"
#include "init.hpp"
#include "input.hpp"
#include "minimap.hpp"
#include "render.hpp"
#include "scaling.hpp"
#include "tag_args.hpp"
#include "tag_peer.hpp"
#include "text.hpp"
#include "zmq_transport.hpp"

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

// Joins the mesh: binds this Peer's port and subscribes to every other Peer
std::shared_ptr<Transport> connectToPeers(const TagConfig& config) {
    std::map<int, std::string> otherPeers;
    for (int id = 1; id <= arena::MAX_PLAYERS; ++id) {
        if (id != config.peerId) otherPeers[id] = config.peerAddress(id);
    }
    try {
        return std::make_shared<ZmqTransport>(config.peerId, otherPeers);
    } catch (const zmq::error_t& error) {
        std::cerr << "Can't listen on port " << ZmqTransport::port(config.peerId) << ": "
                  << error.what() << " (is another Peer already using --id " << config.peerId
                  << "?)\n";
        return nullptr;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    const TagArgs args = parseTagArgs(argc, argv);
    if (args.helpRequested) {
        std::cout << tagUsage();
        return 0;
    }
    if (!args.error.empty()) {
        std::cerr << "nova2d_tag: " << args.error << "\n" << tagUsage();
        return 2;
    }
    const TagConfig& config = args.config;

    // If set, save a screenshot of the game after about one second (or the given number of
    // frames), then quit
    const char* screenshotPath = std::getenv("NOVA2D_TAG_SCREENSHOT");
    const char* screenshotFrames = std::getenv("NOVA2D_TAG_SCREENSHOT_FRAMES");
    const int screenshotFrame = screenshotFrames != nullptr ? std::atoi(screenshotFrames) : 60;
    int frame = 0;

    auto transport = connectToPeers(config);
    if (!transport) return 1;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    const std::string title = "Nova2D - Tag (Peer " + std::to_string(config.peerId) + ")";
    if (!init(window, renderer, title.c_str(), config.windowWidth, config.windowHeight)) {
        return 1;
    }

    TagPeer peer(config, transport);

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
        {
            std::lock_guard<std::mutex> lock(peer.stateMutex());
            renderEntities(renderer, peer.game());
            hud::drawAbilityBars(renderer, peer.game(), arena::playerEntityId(peer.peerId()),
                                 static_cast<int>(peer.game().getCamera().getX()));
            minimap::draw(renderer, peer.game());
        }
        // HUD placeholder, in screen space
        drawText(renderer, "TAG", 16, 16, 4, SDL_Color{255, 255, 255, 255});
        drawText(renderer, "NOVA2D LEGACY ENGINE", 16, 52, 2, SDL_Color{255, 255, 255, 255});
        if (screenshotPath != nullptr && ++frame == screenshotFrame) {
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

    // A clean quit: the other Peers remove this player at once
    peer.leave();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
