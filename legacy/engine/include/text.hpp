#pragma once

#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <string>
#include <vector>

// Built-in 5x7 pixel font for HUD text. Covers A-Z, 0-9, space and ": . - / ! ? ×".
// Lowercase letters are drawn as uppercase. Any other character is drawn as a placeholder box.
// Text is drawn in screen space: it ignores the Camera.

// Width and height of one glyph in font pixels, and the gap between glyphs.
constexpr int GLYPH_WIDTH = 5;
constexpr int GLYPH_HEIGHT = 7;
constexpr int GLYPH_SPACING = 1;

// True if the font has its own glyph for the character (lowercase letters count).
bool hasGlyph(char32_t c);

// The filled rectangles that make up the UTF-8 text, with its top-left corner at (x, y).
// Each font pixel becomes a scale x scale square, so text stays crisp at any whole-number scale.
std::vector<SDL_Rect> textRects(const std::string& text, int x, int y, int scale);

// Width in screen pixels of the UTF-8 text at the given scale, e.g. for centring it.
int textWidth(const std::string& text, int scale);

// Draws the UTF-8 text in the colour, with its top-left corner at (x, y) in screen pixels.
void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale,
              SDL_Color color);
