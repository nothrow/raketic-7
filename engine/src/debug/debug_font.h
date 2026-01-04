#pragma once

#include "core/core.h"

void debug_font_draw_char(float x, float y, char c, float scale, color_t color);
void debug_font_draw_string(float x, float y, const char* str, float scale, color_t color);
void debug_font_draw_float(float x, float y, float value, int precision, float scale, color_t color);
