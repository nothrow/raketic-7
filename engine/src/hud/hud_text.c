#include "hud_text.h"
#include "debug/debug_font.h"

// Stub implementation — delegates to debug_font.
// Replace these bodies with your own font renderer.

#define GLYPH_WIDTH 6.0f

void hud_text_draw_string(float x, float y, const char* str, float scale, color_t color) {
  debug_font_draw_string(x, y, str, scale, color);
}

void hud_text_draw_int(float x, float y, int value, float scale, color_t color) {
  debug_font_draw_float(x, y, (float)value, 0, scale, color);
}

void hud_text_draw_float(float x, float y, float value, int precision, float scale, color_t color) {
  debug_font_draw_float(x, y, value, precision, scale, color);
}

float hud_text_string_width(const char* str, float scale) {
  int len = 0;
  while (str[len]) len++;
  return (float)len * GLYPH_WIDTH * scale;
}
