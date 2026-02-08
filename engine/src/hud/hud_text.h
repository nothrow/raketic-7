#pragma once

#include "core/core.h"

// HUD bitmask font: each glyph is a square with edges + diagonals.
// Case-insensitive letters, digits, and basic punctuation.

void hud_text_draw_string(float x, float y, const char* str, float scale, color_t color);
void hud_text_draw_int(float x, float y, int value, float scale, color_t color);
void hud_text_draw_float(float x, float y, float value, int precision, float scale, color_t color);

// Returns the width of a string in pixels at the given scale.
float hud_text_string_width(const char* str, float scale);
