#pragma once
#include <cmath>

namespace Nova {

struct Vec2 {
    float x{0.f}, y{0.f};

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(const Vec2& o) {
        x += o.x;
        y += o.y;
        return *this;
    }
    Vec2& operator-=(const Vec2& o) {
        x -= o.x;
        y -= o.y;
        return *this;
    }
    Vec2& operator*=(float s) {
        x *= s;
        y *= s;
        return *this;
    }

    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2& o) const { return !(*this == o); }

    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSq() const { return x * x + y * y; }
    float dot(const Vec2& o) const { return x * o.x + y * o.y; }

    Vec2 normalised() const {
        float len = length();
        return len > 1e-6f ? *this / len : Vec2{};
    }

    static Vec2 zero() { return {0.f, 0.f}; }
    static Vec2 one() { return {1.f, 1.f}; }
    static Vec2 up() { return {0.f, -1.f}; }  // SDL y-axis points down
    static Vec2 down() { return {0.f, 1.f}; }
    static Vec2 left() { return {-1.f, 0.f}; }
    static Vec2 right() { return {1.f, 0.f}; }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }

}  // namespace Nova