#include "timeline.hpp"

#include <chrono>
#include <mutex>

Timeline::Timeline(Timeline* anchor, int64_t tic)
    : elapsed_paused_time(0), last_paused_time(0), tic(tic), paused(false), anchor(anchor) {
    start_time = getCurrentTime();
}

Timeline::Timeline() : Timeline(nullptr, 1'000'000) {}  // Default constructor

int64_t Timeline::getCurrentTime() {
    if (anchor != nullptr) {
        // Get current time from the anchor timeline
        return anchor->getCurrentTime();
    } else {
        // Get current time from the system clock (steady_clock)
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }
}

int64_t Timeline::getTime() {
    // std::lock_guard<std::mutex> lock(m);

    int64_t current_time;
    if (paused) {
        // If paused, use the time when it was paused
        current_time = last_paused_time;
    } else {
        // Get the current time from the anchor or system clock
        current_time = getCurrentTime();
    }

    // Calculate elapsed time excluding paused duration
    int64_t elapsed_time = current_time - start_time - elapsed_paused_time;

    // Return elapsed time divided by tic size to get game time units
    return elapsed_time / tic;
}

void Timeline::pause() {
    // std::lock_guard<std::mutex> lock(m);

    if (!paused) {
        // Record the time when paused
        last_paused_time = getCurrentTime();
        paused = true;
    }
}

void Timeline::unpause() {
    // std::lock_guard<std::mutex> lock(m);

    if (paused) {
        // Calculate the duration spent paused and update elapsed_paused_time
        int64_t now = getCurrentTime();
        elapsed_paused_time += now - last_paused_time;
        paused = false;
    }
}

void Timeline::changeTic(int64_t new_tic) {
    // std::lock_guard<std::mutex> lock(m);

    // Adjust start_time to maintain continuity of game time
    int64_t current_game_time = getTime();
    int64_t current_time = getCurrentTime();
    start_time = current_time - (current_game_time * new_tic + elapsed_paused_time);

    // Update tic size
    tic = new_tic;
}

bool Timeline::isPaused() const {
    // std::lock_guard<std::mutex> lock(m);
    return paused;
}
