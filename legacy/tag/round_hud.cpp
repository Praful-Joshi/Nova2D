#include "round_hud.hpp"

#include <algorithm>
#include <vector>

#include "arena.hpp"
#include "text.hpp"

namespace hud {

namespace {

constexpr SDL_Color WHITE{255, 255, 255, 255};
constexpr SDL_Color SHADOW{0, 0, 40, 255};
constexpr SDL_Color HIGHLIGHT{255, 220, 80, 255};
constexpr SDL_Color IT_RED{240, 60, 60, 255};

// Text with a drop shadow so it reads against any background
void drawLabel(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale,
               SDL_Color color = WHITE) {
    drawText(renderer, text, x + scale / 2 + 1, y + scale / 2 + 1, scale, SHADOW);
    drawText(renderer, text, x, y, scale, color);
}

void drawCentred(SDL_Renderer* renderer, const std::string& text, int centreX, int y, int scale,
                 SDL_Color color = WHITE) {
    drawLabel(renderer, text, centreX - textWidth(text, scale) / 2, y, scale, color);
}

void drawLobby(SDL_Renderer* renderer, TagPeer& peer, int centreX, int top) {
    drawCentred(renderer, "LOBBY", centreX, top, 6);
    const std::vector<int> present = peer.presentPeers();
    int y = top + 70;
    for (int id : present) {
        std::string line = "PEER " + std::to_string(id);
        if (id == peer.hostId()) line += " - HOST";
        if (id == peer.peerId()) line += " - YOU";
        drawCentred(renderer, line, centreX, y, 3, id == peer.peerId() ? HIGHLIGHT : WHITE);
        y += 32;
    }

    const int expected = peer.config().players;
    const int missing = expected - static_cast<int>(present.size());
    if (expected < 2) {
        drawCentred(renderer, "PRACTICE - TAG NEEDS 2 TO 4 PLAYERS", centreX, y + 16, 2);
    } else if (missing > 0) {
        drawCentred(renderer,
                    "WAITING FOR " + std::to_string(missing) +
                        (missing == 1 ? " MORE PLAYER" : " MORE PLAYERS"),
                    centreX, y + 16, 2);
    }
}

// Seconds to one decimal place, e.g. "42.5 S"
std::string formatItTime(int64_t ms) {
    const int64_t tenths = (ms + 50) / 100;
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) + " S";
}

// Who is It, top left, and whether this Peer's player has Tag-back Immunity
void drawWhoIsIt(SDL_Renderer* renderer, TagPeer& peer, int y) {
    const int it = peer.itPeerId();
    if (it == 0) return;
    if (it == peer.peerId()) {
        drawLabel(renderer, peer.isFrozen(it) ? "YOU ARE IT - FROZEN" : "YOU ARE IT", 16, y, 2,
                  IT_RED);
    } else {
        drawLabel(renderer, "PEER " + std::to_string(it) + " IS IT", 16, y, 2, IT_RED);
    }
    if (peer.isImmune(peer.peerId())) {
        drawLabel(renderer, "TAG-BACK IMMUNE", 16, y + 24, 2, HIGHLIGHT);
    }
}

// Each participant's It time and the winner: the least time as It
void drawResults(SDL_Renderer* renderer, TagPeer& peer, int centreX, int top) {
    drawCentred(renderer, "ROUND OVER", centreX, top, 6);
    const std::vector<int> winners = peer.winners();
    const auto isWinner = [&winners](int id) {
        return std::find(winners.begin(), winners.end(), id) != winners.end();
    };

    int y = top + 64;
    drawCentred(renderer, "TIME AS IT", centreX, y, 2);
    y += 30;
    for (const ItTime& itTime : peer.itTimes()) {
        const int id = itTime.peerId;
        std::string line = "PEER " + std::to_string(id) + "   " + formatItTime(itTime.ms);
        if (id == peer.peerId()) line += " - YOU";
        drawCentred(renderer, line, centreX, y, 3,
                    isWinner(id) ? HIGHLIGHT : arena::playerColor(id));
        y += 30;
    }

    if (winners.size() == 1) {
        const bool you = winners.front() == peer.peerId();
        drawCentred(renderer,
                    you ? "YOU WIN!" : "PEER " + std::to_string(winners.front()) + " WINS!",
                    centreX, y + 10, 4, HIGHLIGHT);
    } else if (winners.size() > 1) {
        drawCentred(renderer, "DRAW!", centreX, y + 10, 4, HIGHLIGHT);
    }
    drawCentred(renderer,
                "NEXT ROUND IN " + std::to_string((peer.rounds().remainingMs() + 999) / 1000),
                centreX, y + 60, 2);
}

}  // namespace

std::string formatRoundTime(int64_t remainingMs) {
    const int64_t seconds = (remainingMs + 999) / 1000;
    const int64_t secondsPart = seconds % 60;
    return std::to_string(seconds / 60) + ":" + (secondsPart < 10 ? "0" : "") +
           std::to_string(secondsPart);
}

void drawRoundHud(SDL_Renderer* renderer, TagPeer& peer) {
    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    const int centreX = width / 2;
    const Rounds& rounds = peer.rounds();

    // Who and where this Peer is, top left
    std::string identity = "PEER " + std::to_string(peer.peerId());
    if (peer.isHost()) identity += " - HOST";
    drawLabel(renderer, identity, 16, 16, 2, arena::playerColor(peer.peerId()));
    if (rounds.round() > 0) {
        drawLabel(renderer, "ROUND " + std::to_string(rounds.round()), 16, 40, 2);
    }

    switch (rounds.phase()) {
        case RoundPhase::LOBBY:
            drawLobby(renderer, peer, centreX, height / 4);
            break;
        case RoundPhase::COUNTDOWN:
            drawCentred(renderer, std::to_string((rounds.remainingMs() + 999) / 1000), centreX,
                        height / 3, 16);
            break;
        case RoundPhase::PLAYING:
            drawLabel(renderer, formatRoundTime(rounds.remainingMs()),
                      width - 16 - textWidth("0:00", 4), 16, 4);
            drawWhoIsIt(renderer, peer, 64);
            break;
        case RoundPhase::RESULTS:
            drawResults(renderer, peer, centreX, height / 4);
            break;
    }

    if (const std::optional<int> newHost = peer.hostChangeNotice()) {
        drawCentred(renderer, "PEER " + std::to_string(*newHost) + " IS NOW HOST", centreX, 140, 3,
                    HIGHLIGHT);
    }

    if (rounds.isSpectator(peer.peerId())) {
        drawCentred(renderer, "SPECTATING - YOU PLAY NEXT ROUND", centreX, height - 60, 2);
    }
}

}  // namespace hud
