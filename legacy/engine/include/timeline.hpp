#ifndef TIMELINE_HPP
#define TIMELINE_HPP

#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <cstdint>
#include <functional>

// Game time that can be paused and run faster or slower than its source. A root Timeline's
// source is a clock of real time in nanoseconds; an anchored Timeline's source is its anchor's
// game time, so pausing or slowing the anchor pauses or slows it too. Pausing, unpausing and
// changing tic never make game time jump.
class Timeline {
public:
    // A source of real time in nanoseconds
    using Clock = std::function<int64_t()>;

    // Root timeline driven by the given clock; tic is nanoseconds per game unit
    explicit Timeline(Clock clock, int64_t tic = 1'000'000);
    // Timeline anchored to another timeline; tic is anchor units per game unit
    Timeline(Timeline& anchor, int64_t tic);
    // Root timeline driven by the system clock, in game milliseconds
    Timeline();

    // Game time, in game units
    int64_t getTime();
    // The source's time now: the clock's, or the anchor's game time
    int64_t getCurrentTime();
    void pause();
    void unpause();
    // Changes the speed: source units per game unit (smaller is faster)
    void changeTic(int64_t newTic);
    bool isPaused() const;
    int64_t getTic() const { return tic; }

private:
    // Source units run since game_time was last updated, under the current tic
    int64_t runningUnits();
    // Folds the time run so far into game_time and carry
    void rebase();

    int64_t game_time = 0;   // game units at the last rebase
    int64_t carry = 0;       // source units towards the next game unit at the last rebase
    int64_t rebased_at = 0;  // source time of the last rebase or unpause
    int64_t tic;
    bool paused = false;
    Timeline* anchor = nullptr;
    Clock clock;
};

#endif
