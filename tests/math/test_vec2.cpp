#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "math/Vec2.hpp"

using Catch::Approx;
using Nova::Vec2;

TEST_CASE("Vec2 default constructs to zero", "[math][vec2]") {
    Vec2 v;
    REQUIRE(v.x == 0.f);
    REQUIRE(v.y == 0.f);
}

TEST_CASE("Vec2 addition", "[math][vec2]") {
    Vec2 a{1.f, 2.f}, b{3.f, 4.f};
    Vec2 c = a + b;
    REQUIRE(c.x == 4.f);
    REQUIRE(c.y == 6.f);
}

TEST_CASE("Vec2 scalar multiplication", "[math][vec2]") {
    Vec2 v{3.f, 4.f};
    Vec2 r = v * 2.f;
    REQUIRE(r.x == 6.f);
    REQUIRE(r.y == 8.f);
}

TEST_CASE("Vec2 length", "[math][vec2]") {
    Vec2 v{3.f, 4.f};
    REQUIRE(v.length() == Approx(5.f));
}

TEST_CASE("Vec2 normalised has unit length", "[math][vec2]") {
    Vec2 v{3.f, 4.f};
    Vec2 n = v.normalised();
    REQUIRE(n.length() == Approx(1.f));
}

TEST_CASE("Vec2 normalised of zero vector returns zero", "[math][vec2]") {
    Vec2 v{0.f, 0.f};
    Vec2 n = v.normalised();
    REQUIRE(n.x == 0.f);
    REQUIRE(n.y == 0.f);
}

TEST_CASE("Vec2 dot product", "[math][vec2]") {
    Vec2 a{1.f, 0.f}, b{0.f, 1.f};
    REQUIRE(a.dot(b) == Approx(0.f));  // perpendicular

    Vec2 c{1.f, 0.f}, d{1.f, 0.f};
    REQUIRE(c.dot(d) == Approx(1.f));  // parallel
}