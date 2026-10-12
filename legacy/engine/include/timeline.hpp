#ifndef TIMELINE_HPP
#define TIMELINE_HPP

#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <cstdint>
#include <functional>

class Timeline {
public:
    // A source of real time in nanoseconds
    using Clock = std::function<int64_t()>;

    // Root timeline driven by the given clock (the system clock by default)
    explicit Timeline(Clock clock, int64_t tic = 1'000'000);
    // Timeline anchored to another timeline
    Timeline(Timeline* anchor, int64_t tic);
    Timeline();

    int64_t getTime();
    int64_t getCurrentTime();
    void pause();
    void unpause();
    void changeTic(int64_t newTic);
    bool isPaused() const;
    int64_t getTic() const { return tic; }

private:
    int64_t start_time;
    int64_t elapsed_paused_time;
    int64_t last_paused_time;
    int64_t tic;
    bool paused = false;
    Timeline* anchor;
    Clock clock;
};

#endif
