#include "timeline_hud.hpp"

#include "text.hpp"

namespace hud {

namespace {

constexpr int64_t NORMAL_TIC = 1'000'000;
constexpr SDL_Color WHITE{255, 255, 255, 255};
constexpr SDL_Color SHADOW{0, 0, 40, 255};

// Text with a drop shadow so it reads against any background
void drawLabel(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale) {
    drawText(renderer, text, x + scale / 2 + 1, y + scale / 2 + 1, scale, SHADOW);
    drawText(renderer, text, x, y, scale, WHITE);
}

}  // namespace

std::string pauseNotice(bool paused, bool isHost) {
    if (!paused) return "";
    return isHost ? "PAUSED" : "PAUSED BY HOST";
}

std::string speedBadge(int64_t tic) {
    if (tic == NORMAL_TIC || tic <= 0) return "";
    // Speed is NORMAL_TIC / tic; whole speeds read "2×", halves "0.5×"
    const int64_t tenths = (NORMAL_TIC * 10 + tic / 2) / tic;
    std::string speed = std::to_string(tenths / 10);
    if (tenths % 10 != 0) speed += "." + std::to_string(tenths % 10);
    return speed + "\xC3\x97";  // ×
}

void drawTimelineHud(SDL_Renderer* renderer, TagPeer& peer) {
    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    const std::string badge = speedBadge(peer.timeline().getTic());
    if (!badge.empty()) {
        drawLabel(renderer, badge, width - 16 - textWidth(badge, 3), 56, 3);
    }
    const std::string notice = pauseNotice(peer.timeline().isPaused(), peer.isHost());
    if (!notice.empty()) {
        drawLabel(renderer, notice, (width - textWidth(notice, 5)) / 2, height / 2 + 80, 5);
    }
}

}  // namespace hud
