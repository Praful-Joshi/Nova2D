#pragma once
#include <algorithm>
#include <cmath>

namespace Nova::Math {

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = PI * 2.f;
constexpr float HALF_PI = PI * 0.5f;
constexpr float DEG2RAD = PI / 180.f;
constexpr float RAD2DEG = 180.f / PI;

template <typename T>
T clamp(T val, T lo, T hi) {
    return std::max(lo, std::min(val, hi));
}

template <typename T>
T lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

inline float toRadians(float deg) { return deg * DEG2RAD; }
inline float toDegrees(float rad) { return rad * RAD2DEG; }

inline bool nearlyEqual(float a, float b, float eps = 1e-5f) { return std::abs(a - b) < eps; }

}  // namespace Nova::Math