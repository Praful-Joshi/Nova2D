#ifndef TIMELINE_HPP
#define TIMELINE_HPP

#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <stdio.h>

#include <sstream>
#include <string>
#include <vector>

class Timeline {
private:
    int64_t start_time;
    int64_t elapsed_paused_time;
    int64_t last_paused_time;
    int64_t tic;
    bool paused = false;
    Timeline* anchor;

public:
    Timeline(Timeline* anchor, int64_t tic);
    Timeline();
    int64_t getTime();
    int64_t getCurrentTime();
    void pause();
    void unpause();
    void changeTic(int64_t newTic);
    bool isPaused() const;
};

#endif
