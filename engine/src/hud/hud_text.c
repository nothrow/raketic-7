#include "hud_text.h"
#include "platform/platform.h"

// ============================================================================
// Bitmask font: each glyph is a square with 6 possible segments.
//
//   0 -------- 1       bit 0 (0x01): edge 0→1 (top)
//   | ╲      ╱ |       bit 1 (0x02): edge 1→2 (right)
//   |   ╲  ╱   |       bit 2 (0x04): edge 2→3 (bottom)
//   |    ╳     |       bit 3 (0x08): edge 3→0 (left)
//   |  ╱   ╲   |       bit 4 (0x10): diagonal 0→2
//   |╱       ╲ |       bit 5 (0x20): diagonal 1→3
//   3 -------- 2
//
// ============================================================================

// A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
static const uint8_t _letter_map[26] = {
  43, 61, 13, 28, 45, 25, 29, 24, 32, 6, 56, 12, 59, 26, 15, 27, 43, 57, 21, 44, 14, 40, 62, 48, 22, 37
};

// 0  1  2  3  4  5  6  7  8  9
static const uint8_t _digit_map[10] = {
  15, 32, 37, 55, 42, 21, 29, 3, 53, 23
};

static const uint8_t _glyph_slash   = 16;  // diagonal 0→2
static const uint8_t _glyph_unknown = 5;   // top + bottom

// Glyph square side and character advance (at scale 1.0)
#define GLYPH_SIZE    7.0f
#define GLYPH_ADVANCE 11.0f

// ============================================================================
// Core glyph renderer
// ============================================================================

static void _draw_glyph(uint8_t glyph, float x, float y, float size, color_t c) {
  float x0 = x, y0 = y;
  float x1 = x + size, y1 = y;
  float x2 = x + size, y2 = y + size;
  float x3 = x, y3 = y + size;

  if (glyph & 0x01) platform_renderer_draw_line(x0, y0, x1, y1, c);
  if (glyph & 0x02) platform_renderer_draw_line(x1, y1, x2, y2, c);
  if (glyph & 0x04) platform_renderer_draw_line(x2, y2, x3, y3, c);
  if (glyph & 0x08) platform_renderer_draw_line(x3, y3, x0, y0, c);
  if (glyph & 0x10) platform_renderer_draw_line(x0, y0, x2, y2, c);
  if (glyph & 0x20) platform_renderer_draw_line(x1, y1, x3, y3, c);
}

// Special characters that need custom drawing (not expressible as bitmask)

static void _draw_period(float x, float y, float size, color_t c) {
  // Small dot at bottom-center
  float cx = x + size * 0.5f;
  float by = y + size;
  platform_renderer_draw_line(cx - 1, by, cx + 1, by, c);
}

static void _draw_minus(float x, float y, float size, color_t c) {
  // Horizontal line at vertical center
  float my = y + size * 0.5f;
  platform_renderer_draw_line(x, my, x + size, my, c);
}

static void _draw_colon(float x, float y, float size, color_t c) {
  // Two dots at 1/3 and 2/3 height
  float cx = x + size * 0.5f;
  float y1 = y + size * 0.3f;
  float y2 = y + size * 0.7f;
  platform_renderer_draw_line(cx - 1, y1, cx + 1, y1, c);
  platform_renderer_draw_line(cx - 1, y2, cx + 1, y2, c);
}

// ============================================================================
// Character renderer
// ============================================================================

static void _draw_char(char ch, float x, float y, float size, color_t c) {
  if (ch >= 'A' && ch <= 'Z') {
    _draw_glyph(_letter_map[ch - 'A'], x, y, size, c);
  } else if (ch >= 'a' && ch <= 'z') {
    _draw_glyph(_letter_map[ch - 'a'], x, y, size, c);
  } else if (ch >= '0' && ch <= '9') {
    _draw_glyph(_digit_map[ch - '0'], x, y, size, c);
  } else if (ch == '/') {
    _draw_glyph(_glyph_slash, x, y, size, c);
  } else if (ch == '.') {
    _draw_period(x, y, size, c);
  } else if (ch == '-') {
    _draw_minus(x, y, size, c);
  } else if (ch == ':') {
    _draw_colon(x, y, size, c);
  } else if (ch == ' ') {
    // blank
  } else {
    _draw_glyph(_glyph_unknown, x, y, size, c);
  }
}

// ============================================================================
// Public API
// ============================================================================

void hud_text_draw_string(float x, float y, const char* str, float scale, color_t color) {
  float size = GLYPH_SIZE * scale;
  float advance = GLYPH_ADVANCE * scale;

  while (*str) {
    _draw_char(*str, x, y, size, color);
    x += advance;
    str++;
  }
}

void hud_text_draw_int(float x, float y, int value, float scale, color_t color) {
  char buf[16];
  int i = 0;

  if (value < 0) { buf[i++] = '-'; value = -value; }
  if (value == 0) {
    buf[i++] = '0';
  } else {
    int digits[10], nd = 0;
    while (value > 0) { digits[nd++] = value % 10; value /= 10; }
    for (int d = nd - 1; d >= 0; d--) buf[i++] = (char)('0' + digits[d]);
  }
  buf[i] = 0;

  hud_text_draw_string(x, y, buf, scale, color);
}

void hud_text_draw_float(float x, float y, float value, int precision, float scale, color_t color) {
  char buf[32];
  int neg = value < 0;
  if (neg) value = -value;

  int whole = (int)value;
  float frac = value - (float)whole;

  int i = 0;
  if (neg) buf[i++] = '-';

  int digits[16], nd = 0;
  if (whole == 0) {
    digits[nd++] = 0;
  } else {
    while (whole > 0) { digits[nd++] = whole % 10; whole /= 10; }
  }
  for (int j = nd - 1; j >= 0; j--) buf[i++] = (char)('0' + digits[j]);

  if (precision > 0) {
    buf[i++] = '.';
    for (int p = 0; p < precision; p++) {
      frac *= 10.0f;
      int d = (int)frac;
      buf[i++] = (char)('0' + d);
      frac -= (float)d;
    }
  }
  buf[i] = 0;

  hud_text_draw_string(x, y, buf, scale, color);
}

float hud_text_string_width(const char* str, float scale) {
  int len = 0;
  while (str[len]) len++;
  return (float)len * GLYPH_ADVANCE * scale;
}
