#include "timeline.hpp"

#include <chrono>
#include <utility>

namespace {
int64_t systemNanoseconds() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
}  // namespace

Timeline::Timeline(Clock clock, int64_t tic) : tic(tic), clock(std::move(clock)) {
    rebased_at = getCurrentTime();
}

Timeline::Timeline(Timeline& anchor, int64_t tic) : tic(tic), anchor(&anchor) {
    rebased_at = getCurrentTime();
}

Timeline::Timeline() : Timeline(Clock(systemNanoseconds)) {}

int64_t Timeline::getCurrentTime() { return anchor != nullptr ? anchor->getTime() : clock(); }

int64_t Timeline::runningUnits() {
    // Nothing runs while paused
    return carry + (paused ? 0 : getCurrentTime() - rebased_at);
}

int64_t Timeline::getTime() { return game_time + runningUnits() / tic; }

void Timeline::rebase() {
    const int64_t units = runningUnits();
    game_time += units / tic;
    carry = units % tic;
    rebased_at = getCurrentTime();
}

void Timeline::pause() {
    if (paused) return;
    rebase();
    paused = true;
}

void Timeline::unpause() {
    if (!paused) return;
    // Time spent paused doesn't count
    rebased_at = getCurrentTime();
    paused = false;
}

void Timeline::changeTic(int64_t newTic) {
    rebase();
    // The part-elapsed game unit stays the same fraction of a unit
    carry = carry * newTic / tic;
    tic = newTic;
}

bool Timeline::isPaused() const { return paused; }
