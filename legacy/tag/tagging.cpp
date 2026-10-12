#include "tagging.hpp"

#include <algorithm>

#include "arena.hpp"

namespace tagging {

namespace {

// Whether two players' rects overlap, using their exact World Coordinates
bool overlaps(const Entity& a, const Entity& b) {
    return a.getX() < b.getX() + b.getRect().w && b.getX() < a.getX() + a.getRect().w &&
           a.getY() < b.getY() + b.getRect().h && b.getY() < a.getY() + a.getRect().h;
}

// Calls f(peerId, tag) for every player in the game
template <typename F>
void forEachPlayer(const Game& game, F f) {
    for (int peerId = 1; peerId <= arena::MAX_PLAYERS; ++peerId) {
        if (auto tag = playerTag(game, peerId)) f(peerId, *tag);
    }
}

}  // namespace

std::shared_ptr<TagPlayerAttribute> playerTag(const Game& game, int peerId) {
    return game.getAttribute<TagPlayerAttribute>(Property::Custom, arena::playerEntityId(peerId));
}

int it(const Game& game) {
    int itId = 0;
    forEachPlayer(game, [&](int peerId, const TagPlayerAttribute& tag) {
        if (tag.it && itId == 0) itId = peerId;
    });
    return itId;
}

void reset(Game& game) {
    forEachPlayer(game, [](int, TagPlayerAttribute& tag) {
        tag.it = false;
        tag.frozenUntil = 0;
        tag.immuneUntil = 0;
        tag.itTimeMs = 0;
    });
}

void makeIt(Game& game, int peerId, int64_t frozenUntil) {
    forEachPlayer(game, [&](int id, TagPlayerAttribute& tag) {
        tag.it = id == peerId;
        tag.frozenUntil = id == peerId ? frozenUntil : 0;
        tag.immuneUntil = 0;
    });
}

void tag(Game& game, int from, int to, int64_t immuneUntil) {
    forEachPlayer(game, [&](int id, TagPlayerAttribute& tag) {
        tag.it = id == to;
        tag.frozenUntil = 0;
        tag.immuneUntil = id == from ? immuneUntil : 0;
    });
}

int findTagging(Game& game, const std::vector<int>& participants, int64_t now) {
    const int itId = it(game);
    auto itPlayer = game.getEntity(arena::playerEntityId(itId));
    auto itTag = playerTag(game, itId);
    if (!itPlayer || !itTag || !itPlayer->isActive() || itTag->isFrozen(now)) return 0;

    for (int peerId : participants) {
        if (peerId == itId) continue;
        auto player = game.getEntity(arena::playerEntityId(peerId));
        auto tag = playerTag(game, peerId);
        if (!player || !tag || !player->isActive() || tag->spectator || tag->isImmune(now)) {
            continue;
        }
        if (overlaps(*itPlayer, *player)) return peerId;
    }
    return 0;
}

std::vector<ItTime> itTimes(const Game& game, const std::vector<int>& participants) {
    std::vector<ItTime> times;
    for (int peerId : participants) {
        if (auto tag = playerTag(game, peerId)) times.push_back(ItTime{peerId, tag->itTimeMs});
    }
    return times;
}

void setItTimes(Game& game, const std::vector<ItTime>& itTimes) {
    for (const ItTime& itTime : itTimes) {
        if (auto tag = playerTag(game, itTime.peerId)) tag->itTimeMs = itTime.ms;
    }
}

void writeHostState(const Game& game, const std::vector<int>& participants, int64_t now,
                    HostState& state) {
    state.it = it(game);
    forEachPlayer(game, [&](int peerId, const TagPlayerAttribute& tag) {
        if (peerId == state.it) state.itFrozenMs = std::max<int64_t>(0, tag.frozenUntil - now);
        if (tag.isImmune(now)) {
            state.immune = peerId;
            state.immuneMs = tag.immuneUntil - now;
        }
    });
    state.itTimes = itTimes(game, participants);
}

void followHostState(Game& game, const HostState& state, int64_t now) {
    makeIt(game, state.it, now + state.itFrozenMs);
    if (auto tag = playerTag(game, state.immune)) tag->immuneUntil = now + state.immuneMs;
    setItTimes(game, state.itTimes);
}

std::vector<int> winners(const Game& game, const std::vector<int>& participants) {
    const std::vector<ItTime> times = itTimes(game, participants);
    if (times.empty()) return {};
    const int64_t least =
        std::min_element(times.begin(), times.end(), [](const ItTime& a, const ItTime& b) {
            return a.ms < b.ms;
        })->ms;
    std::vector<int> ids;
    for (const ItTime& itTime : times) {
        if (itTime.ms == least) ids.push_back(itTime.peerId);
    }
    return ids;
}

}  // namespace tagging
