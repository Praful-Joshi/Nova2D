#pragma once
#include <cstdint>

namespace Nova {

class Clock {
public:
    static constexpr float FIXED_DT = 1.f / 60.f;  // 60hz physics

    Clock();

    // Call once per frame. Returns real wall-clock delta since last call.
    float tick();

    float deltaTime() const { return m_deltaTime; }
    float accumulator() const { return m_accumulator; }
    float alpha() const { return m_accumulator / FIXED_DT; }
    uint64_t frameCount() const { return m_frameCount; }
    float totalTime() const { return m_totalTime; }

    // Returns true if a fixed update step should run; call in a while loop.
    // Consumes one FIXED_DT from the accumulator each call.
    bool shouldFixedUpdate();

private:
    uint64_t m_lastTick{0};
    float m_deltaTime{0.f};
    float m_accumulator{0.f};
    float m_totalTime{0.f};
    uint64_t m_frameCount{0};
};

}  // namespace Nova