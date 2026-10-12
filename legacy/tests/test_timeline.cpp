#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "timeline.hpp"

namespace {

constexpr int64_t MS = 1'000'000;  // nanoseconds

// A real-time clock the test moves by hand
struct ManualClock {
    int64_t nowNs = 0;

    Timeline::Clock clock() {
        return [this] { return nowNs; };
    }
    void advanceMs(int64_t ms) { nowNs += ms * MS; }
};

}  // namespace

TEST_CASE("A Timeline counts game time in tics of real time", "[legacy][timeline]") {
    ManualClock real;
    real.advanceMs(5'000);  // the clock doesn't start at zero
    Timeline normal(real.clock());
    Timeline fast(real.clock(), 500'000);
    Timeline slow(real.clock(), 2'000'000);
    REQUIRE(normal.getTime() == 0);

    real.advanceMs(1'000);

    REQUIRE(normal.getTime() == 1'000);
    REQUIRE(fast.getTime() == 2'000);
    REQUIRE(slow.getTime() == 500);
}

TEST_CASE("Paused time doesn't advance", "[legacy][timeline]") {
    ManualClock real;
    Timeline timeline(real.clock());
    real.advanceMs(300);

    timeline.pause();
    real.advanceMs(2'000);

    REQUIRE(timeline.isPaused());
    REQUIRE(timeline.getTime() == 300);
}

TEST_CASE("After unpausing, time continues without a jump", "[legacy][timeline]") {
    ManualClock real;
    Timeline timeline(real.clock());
    real.advanceMs(300);
    timeline.pause();
    real.advanceMs(2'000);

    timeline.unpause();
    REQUIRE_FALSE(timeline.isPaused());
    REQUIRE(timeline.getTime() == 300);
    real.advanceMs(100);
    REQUIRE(timeline.getTime() == 400);
}

TEST_CASE("Pausing twice or unpausing a running Timeline changes nothing", "[legacy][timeline]") {
    ManualClock real;
    Timeline timeline(real.clock());
    real.advanceMs(100);
    timeline.unpause();
    real.advanceMs(100);
    REQUIRE(timeline.getTime() == 200);

    timeline.pause();
    real.advanceMs(100);
    timeline.pause();
    real.advanceMs(100);
    timeline.unpause();
    real.advanceMs(100);
    REQUIRE(timeline.getTime() == 300);
}

TEST_CASE("Changing tic keeps game time continuous", "[legacy][timeline]") {
    ManualClock real;
    Timeline timeline(real.clock());
    real.advanceMs(1'000);

    timeline.changeTic(500'000);  // 2x
    REQUIRE(timeline.getTic() == 500'000);
    REQUIRE(timeline.getTime() == 1'000);
    real.advanceMs(1'000);
    REQUIRE(timeline.getTime() == 3'000);

    timeline.changeTic(2'000'000);  // 0.5x
    REQUIRE(timeline.getTime() == 3'000);
    real.advanceMs(1'000);
    REQUIRE(timeline.getTime() == 3'500);
}

TEST_CASE("Changing tic keeps a part-elapsed game unit", "[legacy][timeline]") {
    ManualClock real;
    Timeline timeline(real.clock(), 2'000'000);
    real.nowNs += 3 * MS;  // 1.5 game units
    REQUIRE(timeline.getTime() == 1);

    timeline.changeTic(1'000'000);  // the half unit left is now 0.5 ms of real time
    real.nowNs += MS / 2;
    REQUIRE(timeline.getTime() == 2);
}

TEST_CASE("Changing tic while paused keeps game time and applies after unpausing",
          "[legacy][timeline]") {
    ManualClock real;
    Timeline timeline(real.clock());
    real.advanceMs(1'000);
    timeline.pause();
    real.advanceMs(5'000);

    timeline.changeTic(2'000'000);
    REQUIRE(timeline.isPaused());
    REQUIRE(timeline.getTime() == 1'000);
    real.advanceMs(5'000);
    REQUIRE(timeline.getTime() == 1'000);

    timeline.unpause();
    REQUIRE(timeline.getTime() == 1'000);
    real.advanceMs(1'000);
    REQUIRE(timeline.getTime() == 1'500);
}

TEST_CASE("Changing tic, then pausing and unpausing, keeps game time continuous",
          "[legacy][timeline]") {
    ManualClock real;
    Timeline timeline(real.clock());
    real.advanceMs(1'000);
    timeline.changeTic(500'000);
    real.advanceMs(1'000);  // 3000

    timeline.pause();
    real.advanceMs(4'000);
    REQUIRE(timeline.getTime() == 3'000);
    timeline.unpause();
    real.advanceMs(1'000);
    REQUIRE(timeline.getTime() == 5'000);
}

TEST_CASE("An anchored Timeline runs on its anchor's game time", "[legacy][timeline]") {
    ManualClock real;
    Timeline anchor(real.clock());
    real.advanceMs(1'000);
    Timeline half(anchor, 2);  // one unit per two of the anchor's
    REQUIRE(half.getTime() == 0);

    real.advanceMs(1'000);
    REQUIRE(half.getTime() == 500);

    SECTION("pausing the anchor pauses it") {
        anchor.pause();
        real.advanceMs(1'000);
        REQUIRE(half.getTime() == 500);
        anchor.unpause();
        real.advanceMs(1'000);
        REQUIRE(half.getTime() == 1'000);
    }

    SECTION("changing the anchor's speed changes its speed") {
        anchor.changeTic(500'000);  // anchor at 2x
        real.advanceMs(1'000);
        REQUIRE(half.getTime() == 1'500);
    }

    SECTION("it can be paused and sped up on its own") {
        half.pause();
        real.advanceMs(1'000);
        REQUIRE(anchor.getTime() == 3'000);
        REQUIRE(half.getTime() == 500);
        half.unpause();
        half.changeTic(1);
        real.advanceMs(1'000);
        REQUIRE(half.getTime() == 1'500);
    }
}
