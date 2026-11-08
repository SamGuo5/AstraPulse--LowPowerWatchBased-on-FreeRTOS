#pragma once
#include "u8g2.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace watch_font {
struct Glyph {
  uint16_t codepoint;
  uint16_t bitmap_offset;
  uint8_t width;
  uint8_t height;
  uint8_t row_bytes;
  uint8_t advance;
  int8_t x_offset;
  int8_t y_offset;
};

extern const Glyph glyphs[];
extern const std::size_t glyph_count;
extern const uint8_t bitmap[];

uint16_t utf8_width(const std::string &text);
void draw_utf8(u8g2_t *canvas,int16_t x,int16_t baseline,const std::string &text);
}
