#include "text.hpp"

namespace {

// One character of the built-in font: GLYPH_HEIGHT rows of GLYPH_WIDTH pixels, '#' = filled.
struct Glyph {
    char32_t c;
    const char* rows[GLYPH_HEIGHT];
};

// clang-format off
constexpr Glyph FONT[] = {
    {U'A', {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {U'B', {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}},
    {U'C', {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}},
    {U'D', {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}},
    {U'E', {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}},
    {U'F', {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}},
    {U'G', {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"}},
    {U'H', {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {U'I', {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {U'J', {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."}},
    {U'K', {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}},
    {U'L', {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}},
    {U'M', {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}},
    {U'N', {"#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#"}},
    {U'O', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {U'P', {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}},
    {U'Q', {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}},
    {U'R', {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}},
    {U'S', {".####", "#....", "#....", ".###.", "....#", "....#", "####."}},
    {U'T', {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}},
    {U'U', {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {U'V', {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}},
    {U'W', {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."}},
    {U'X', {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}},
    {U'Y', {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}},
    {U'Z', {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}},
    {U'0', {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."}},
    {U'1', {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {U'2', {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"}},
    {U'3', {"####.", "....#", "....#", ".###.", "....#", "....#", "####."}},
    {U'4', {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}},
    {U'5', {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}},
    {U'6', {"..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."}},
    {U'7', {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}},
    {U'8', {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}},
    {U'9', {".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."}},
    {U' ', {".....", ".....", ".....", ".....", ".....", ".....", "....."}},
    {U':', {".....", "..#..", "..#..", ".....", "..#..", "..#..", "....."}},
    {U'.', {".....", ".....", ".....", ".....", ".....", "..#..", "..#.."}},
    {U'-', {".....", ".....", ".....", "#####", ".....", ".....", "....."}},
    {U'/', {"....#", "....#", "...#.", "..#..", ".#...", "#....", "#...."}},
    {U'!', {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."}},
    {U'?', {".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."}},
    {U'×', {".....", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "....."}},
};

// Drawn for any character the font doesn't cover: a hollow box the size of a glyph.
constexpr Glyph PLACEHOLDER = {
    0, {"#####", "#...#", "#...#", "#...#", "#...#", "#...#", "#####"}};
// clang-format on

char32_t toUpper(char32_t c) { return (c >= U'a' && c <= U'z') ? c - U'a' + U'A' : c; }

const Glyph* findGlyph(char32_t c) {
    c = toUpper(c);
    for (const Glyph& glyph : FONT) {
        if (glyph.c == c) return &glyph;
    }
    return nullptr;
}

// Decodes UTF-8 into code points. A malformed byte becomes U+FFFD, so it draws as the
// placeholder instead of being skipped or crashing.
std::u32string decodeUtf8(const std::string& text) {
    std::u32string out;
    size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        int length = 0;
        char32_t c = 0;
        if (lead < 0x80) {
            length = 1;
            c = lead;
        } else if ((lead & 0xE0) == 0xC0) {
            length = 2;
            c = lead & 0x1F;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3;
            c = lead & 0x0F;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4;
            c = lead & 0x07;
        }

        bool valid = length > 0 && i + length <= text.size();
        for (int k = 1; valid && k < length; ++k) {
            const auto next = static_cast<unsigned char>(text[i + k]);
            if ((next & 0xC0) != 0x80) {
                valid = false;
            } else {
                c = (c << 6) | (next & 0x3F);
            }
        }

        if (valid) {
            out.push_back(c);
            i += length;
        } else {
            out.push_back(U'�');
            ++i;
        }
    }
    return out;
}

}  // namespace

bool hasGlyph(char32_t c) { return findGlyph(c) != nullptr; }

std::vector<SDL_Rect> textRects(const std::string& text, int x, int y, int scale) {
    std::vector<SDL_Rect> rects;
    int penX = x;
    for (const char32_t c : decodeUtf8(text)) {
        const Glyph* glyph = findGlyph(c);
        if (glyph == nullptr) glyph = &PLACEHOLDER;
        for (int row = 0; row < GLYPH_HEIGHT; ++row) {
            for (int col = 0; col < GLYPH_WIDTH; ++col) {
                if (glyph->rows[row][col] != '#') continue;
                rects.push_back({penX + col * scale, y + row * scale, scale, scale});
            }
        }
        penX += (GLYPH_WIDTH + GLYPH_SPACING) * scale;
    }
    return rects;
}

int textWidth(const std::string& text, int scale) {
    const int count = static_cast<int>(decodeUtf8(text).size());
    if (count == 0) return 0;
    return (count * (GLYPH_WIDTH + GLYPH_SPACING) - GLYPH_SPACING) * scale;
}

void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale,
              SDL_Color color) {
    const std::vector<SDL_Rect> rects = textRects(text, x, y, scale);
    if (rects.empty()) return;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRects(renderer, rects.data(), static_cast<int>(rects.size()));
}
