#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "text.hpp"

namespace {

bool sameRects(const std::vector<SDL_Rect>& a, const std::vector<SDL_Rect>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].x != b[i].x || a[i].y != b[i].y || a[i].w != b[i].w || a[i].h != b[i].h) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST_CASE("A dash is a single row of five pixels across the middle of the glyph",
          "[legacy][text]") {
    const std::vector<SDL_Rect> rects = textRects("-", 0, 0, 1);

    REQUIRE(rects.size() == 5);
    for (int i = 0; i < 5; ++i) {
        REQUIRE(rects[i].x == i);
        REQUIRE(rects[i].y == 3);
        REQUIRE(rects[i].w == 1);
        REQUIRE(rects[i].h == 1);
    }
}

TEST_CASE("An unsupported character draws as a hollow placeholder box", "[legacy][text]") {
    // A hollow 5x7 box: top and bottom rows of 5, plus 5 rows with both sides lit = 20 pixels
    const std::vector<SDL_Rect> placeholder = textRects("@", 0, 0, 1);
    REQUIRE_FALSE(hasGlyph(U'@'));
    REQUIRE(placeholder.size() == 20);

    REQUIRE(sameRects(textRects("~", 0, 0, 1), placeholder));
    REQUIRE(sameRects(textRects("\xC3\xA9", 0, 0, 1), placeholder));  // é is one character
    REQUIRE(sameRects(textRects("\xFF", 0, 0, 1), placeholder));      // malformed UTF-8
}

TEST_CASE("Every supported character has its own glyph", "[legacy][text]") {
    const std::vector<std::string> supported = {
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
        "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3",
        "4", "5", "6", "7", "8", "9", " ", ":", ".", "-", "/", "!", "?", "\xC3\x97"};
    const std::vector<SDL_Rect> placeholder = textRects("@", 0, 0, 1);

    for (size_t i = 0; i < supported.size(); ++i) {
        INFO("character: '" << supported[i] << "'");
        const std::vector<SDL_Rect> rects = textRects(supported[i], 0, 0, 1);
        REQUIRE_FALSE(sameRects(rects, placeholder));
        for (size_t j = 0; j < i; ++j) {
            INFO("same glyph as: '" << supported[j] << "'");
            REQUIRE_FALSE(sameRects(rects, textRects(supported[j], 0, 0, 1)));
        }
    }

    for (const char32_t c : U"AZ09 :.-/!?×") {
        if (c == 0) continue;
        REQUIRE(hasGlyph(c));
    }
}

TEST_CASE("The multiplication sign is decoded from UTF-8 as one character", "[legacy][text]") {
    // × is a diagonal cross: 2 + 2 + 1 + 2 + 2 pixels over the middle five rows
    REQUIRE(textRects("\xC3\x97", 0, 0, 1).size() == 9);
    REQUIRE(textWidth("\xC3\x97", 1) == 5);
}

TEST_CASE("Scaling by a whole number gives crisp, exactly multiplied pixels", "[legacy][text]") {
    const std::vector<SDL_Rect> rects = textRects("-", 10, 20, 3);

    REQUIRE(rects.size() == 5);
    for (int i = 0; i < 5; ++i) {
        REQUIRE(rects[i].x == 10 + 3 * i);
        REQUIRE(rects[i].y == 20 + 3 * 3);
        REQUIRE(rects[i].w == 3);
        REQUIRE(rects[i].h == 3);
    }

    const std::vector<SDL_Rect> small = textRects("TAG 42", 0, 0, 1);
    const std::vector<SDL_Rect> big = textRects("TAG 42", 0, 0, 4);
    REQUIRE(small.size() == big.size());
    for (size_t i = 0; i < small.size(); ++i) {
        REQUIRE(big[i].x == small[i].x * 4);
        REQUIRE(big[i].y == small[i].y * 4);
        REQUIRE(big[i].w == 4);
        REQUIRE(big[i].h == 4);
    }
}

TEST_CASE("Lowercase letters draw the same as uppercase", "[legacy][text]") {
    REQUIRE(hasGlyph(U'q'));
    REQUIRE(sameRects(textRects("nova2d legacy engine", 5, 7, 2),
                      textRects("NOVA2D LEGACY ENGINE", 5, 7, 2)));
}

TEST_CASE("Text width grows by one glyph and one gap per character", "[legacy][text]") {
    REQUIRE(textWidth("", 1) == 0);
    REQUIRE(textWidth("A", 1) == 5);
    REQUIRE(textWidth("AB", 1) == 11);
    REQUIRE(textWidth("ABC", 1) == 17);
    REQUIRE(textWidth("AB", 3) == 33);
    REQUIRE(textWidth("A@", 1) == textWidth("AB", 1));  // the placeholder takes a glyph's room

    // The second character starts one glyph and one gap after the first
    REQUIRE(textRects("-", 0, 0, 1).front().x == 0);
    REQUIRE(textRects(" -", 0, 0, 1).front().x == 6);
}
