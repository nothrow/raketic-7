#include "debug_font.h"
#include "platform/platform.h"

#define MOVE -128
#define END -127

static const int8_t glyph_space[] = { END };
static const int8_t glyph_excl[] = { 0,0, 0,6, MOVE, 0,8, 0,9, END };
static const int8_t glyph_quote[] = { -1,0, -1,3, MOVE, 1,0, 1,3, END };
static const int8_t glyph_hash[] = { -2,2, 2,2, MOVE, -2,6, 2,6, MOVE, -1,0, -1,8, MOVE, 1,0, 1,8, END };
static const int8_t glyph_dollar[] = { 2,1, -2,3, 2,5, -2,7, MOVE, 0,0, 0,8, END };
static const int8_t glyph_percent[] = { -2,0, 2,8, MOVE, -2,1, -1,1, -1,2, -2,2, -2,1, MOVE, 1,6, 2,6, 2,7, 1,7, 1,6, END };
static const int8_t glyph_amp[] = { 2,8, -1,4, 0,2, -1,0, -2,1, -1,3, 2,6, 2,8, END };
static const int8_t glyph_apos[] = { 0,0, 0,2, END };
static const int8_t glyph_lparen[] = { 1,0, -1,2, -1,6, 1,8, END };
static const int8_t glyph_rparen[] = { -1,0, 1,2, 1,6, -1,8, END };
static const int8_t glyph_star[] = { 0,1, 0,5, MOVE, -2,2, 2,4, MOVE, 2,2, -2,4, END };
static const int8_t glyph_plus[] = { 0,2, 0,6, MOVE, -2,4, 2,4, END };
static const int8_t glyph_comma[] = { 0,7, -1,9, END };
static const int8_t glyph_minus[] = { -2,4, 2,4, END };
static const int8_t glyph_period[] = { -1,7, 1,7, 1,9, -1,9, -1,7, END };
static const int8_t glyph_slash[] = { -2,8, 2,0, END };

static const int8_t glyph_0[] = { -2,0, 2,0, 2,8, -2,8, -2,0, MOVE, -2,8, 2,0, END };
static const int8_t glyph_1[] = { -1,1, 0,0, 0,8, MOVE, -1,8, 1,8, END };
static const int8_t glyph_2[] = { -2,1, -1,0, 1,0, 2,1, 2,3, -2,8, 2,8, END };
static const int8_t glyph_3[] = { -2,0, 2,0, 2,3, 0,4, 2,5, 2,8, -2,8, END };
static const int8_t glyph_4[] = { 1,8, 1,0, -2,5, 2,5, END };
static const int8_t glyph_5[] = { 2,0, -2,0, -2,3, 1,3, 2,4, 2,7, 1,8, -2,8, END };
static const int8_t glyph_6[] = { 2,0, -2,4, -2,8, 2,8, 2,5, -2,4, END };
static const int8_t glyph_7[] = { -2,0, 2,0, 0,8, END };
static const int8_t glyph_8[] = { -2,4, -2,1, 0,0, 2,1, 2,4, -2,4, -2,7, 0,8, 2,7, 2,4, END };
static const int8_t glyph_9[] = { 2,4, -2,4, -2,1, 0,0, 2,1, 2,8, END };

static const int8_t glyph_colon[] = { 0,2, 0,3, MOVE, 0,6, 0,7, END };
static const int8_t glyph_semi[] = { 0,2, 0,3, MOVE, 0,6, -1,8, END };
static const int8_t glyph_less[] = { 2,0, -2,4, 2,8, END };
static const int8_t glyph_equal[] = { -2,3, 2,3, MOVE, -2,5, 2,5, END };
static const int8_t glyph_greater[] = { -2,0, 2,4, -2,8, END };
static const int8_t glyph_quest[] = { -2,1, -1,0, 1,0, 2,1, 2,2, 0,4, 0,6, MOVE, 0,8, 0,8, END };
static const int8_t glyph_at[] = { 2,6, 2,2, 0,2, 0,5, 2,5, 2,0, -2,0, -2,8, 2,8, END };

static const int8_t glyph_A[] = { -2,8, 0,0, 2,8, MOVE, -1,5, 1,5, END };
static const int8_t glyph_B[] = { -2,0, -2,8, 1,8, 2,7, 2,5, 1,4, -2,4, MOVE, -2,0, 1,0, 2,1, 2,3, 1,4, END };
static const int8_t glyph_C[] = { 2,1, 1,0, -1,0, -2,1, -2,7, -1,8, 1,8, 2,7, END };
static const int8_t glyph_D[] = { -2,0, -2,8, 1,8, 2,7, 2,1, 1,0, -2,0, END };
static const int8_t glyph_E[] = { 2,0, -2,0, -2,8, 2,8, MOVE, -2,4, 1,4, END };
static const int8_t glyph_F[] = { 2,0, -2,0, -2,8, MOVE, -2,4, 1,4, END };
static const int8_t glyph_G[] = { 2,1, 1,0, -1,0, -2,1, -2,7, -1,8, 1,8, 2,7, 2,4, 0,4, END };
static const int8_t glyph_H[] = { -2,0, -2,8, MOVE, 2,0, 2,8, MOVE, -2,4, 2,4, END };
static const int8_t glyph_I[] = { -1,0, 1,0, MOVE, 0,0, 0,8, MOVE, -1,8, 1,8, END };
static const int8_t glyph_J[] = { 1,0, 1,7, 0,8, -1,8, -2,7, END };
static const int8_t glyph_K[] = { -2,0, -2,8, MOVE, 2,0, -2,4, 2,8, END };
static const int8_t glyph_L[] = { -2,0, -2,8, 2,8, END };
static const int8_t glyph_M[] = { -2,8, -2,0, 0,4, 2,0, 2,8, END };
static const int8_t glyph_N[] = { -2,8, -2,0, 2,8, 2,0, END };
static const int8_t glyph_O[] = { -1,0, 1,0, 2,1, 2,7, 1,8, -1,8, -2,7, -2,1, -1,0, END };
static const int8_t glyph_P[] = { -2,8, -2,0, 1,0, 2,1, 2,3, 1,4, -2,4, END };
static const int8_t glyph_Q[] = { -1,0, 1,0, 2,1, 2,7, 1,8, -1,8, -2,7, -2,1, -1,0, MOVE, 1,6, 2,8, END };
static const int8_t glyph_R[] = { -2,8, -2,0, 1,0, 2,1, 2,3, 1,4, -2,4, MOVE, 0,4, 2,8, END };
static const int8_t glyph_S[] = { 2,1, 1,0, -1,0, -2,1, -2,3, -1,4, 1,4, 2,5, 2,7, 1,8, -1,8, -2,7, END };
static const int8_t glyph_T[] = { -2,0, 2,0, MOVE, 0,0, 0,8, END };
static const int8_t glyph_U[] = { -2,0, -2,7, -1,8, 1,8, 2,7, 2,0, END };
static const int8_t glyph_V[] = { -2,0, 0,8, 2,0, END };
static const int8_t glyph_W[] = { -2,0, -1,8, 0,4, 1,8, 2,0, END };
static const int8_t glyph_X[] = { -2,0, 2,8, MOVE, 2,0, -2,8, END };
static const int8_t glyph_Y[] = { -2,0, 0,4, 2,0, MOVE, 0,4, 0,8, END };
static const int8_t glyph_Z[] = { -2,0, 2,0, -2,8, 2,8, END };

static const int8_t glyph_lbracket[] = { 1,0, -1,0, -1,8, 1,8, END };
static const int8_t glyph_backslash[] = { -2,0, 2,8, END };
static const int8_t glyph_rbracket[] = { -1,0, 1,0, 1,8, -1,8, END };
static const int8_t glyph_caret[] = { -2,2, 0,0, 2,2, END };
static const int8_t glyph_under[] = { -2,9, 2,9, END };
static const int8_t glyph_grave[] = { -1,0, 1,2, END };

static const int8_t glyph_a[] = { -2,3, 1,3, 2,4, 2,8, -2,8, -2,6, 2,5, END };
static const int8_t glyph_b[] = { -2,0, -2,8, 1,8, 2,7, 2,4, 1,3, -2,3, END };
static const int8_t glyph_c[] = { 2,3, -1,3, -2,4, -2,7, -1,8, 2,8, END };
static const int8_t glyph_d[] = { 2,0, 2,8, -1,8, -2,7, -2,4, -1,3, 2,3, END };
static const int8_t glyph_e[] = { -2,5, 2,5, 2,4, 1,3, -1,3, -2,4, -2,7, -1,8, 2,8, END };
static const int8_t glyph_f[] = { -1,8, -1,2, 0,1, 1,1, MOVE, -2,4, 1,4, END };
static const int8_t glyph_g[] = { -2,10, 1,10, 2,9, 2,3, -1,3, -2,4, -2,7, -1,8, 2,8, END };
static const int8_t glyph_h[] = { -2,0, -2,8, MOVE, -2,4, 1,3, 2,4, 2,8, END };
static const int8_t glyph_i[] = { 0,1, 0,1, MOVE, 0,3, 0,8, END };
static const int8_t glyph_j[] = { 1,1, 1,1, MOVE, 1,3, 1,9, 0,10, -1,10, END };
static const int8_t glyph_k[] = { -2,0, -2,8, MOVE, 2,3, -2,6, 2,8, END };
static const int8_t glyph_l[] = { -1,0, 0,0, 0,7, 1,8, END };
static const int8_t glyph_m[] = { -2,8, -2,3, 0,3, 0,8, MOVE, 0,3, 2,3, 2,8, END };
static const int8_t glyph_n[] = { -2,8, -2,3, 1,3, 2,4, 2,8, END };
static const int8_t glyph_o[] = { -1,3, 1,3, 2,4, 2,7, 1,8, -1,8, -2,7, -2,4, -1,3, END };
static const int8_t glyph_p[] = { -2,10, -2,3, 1,3, 2,4, 2,7, 1,8, -2,8, END };
static const int8_t glyph_q[] = { 2,10, 2,3, -1,3, -2,4, -2,7, -1,8, 2,8, END };
static const int8_t glyph_r[] = { -2,8, -2,3, MOVE, -2,5, 0,3, 2,3, END };
static const int8_t glyph_s[] = { 2,3, -1,3, -2,4, 1,7, 2,7, 1,8, -2,8, END };
static const int8_t glyph_t[] = { 0,0, 0,7, 1,8, 2,8, MOVE, -1,3, 1,3, END };
static const int8_t glyph_u[] = { -2,3, -2,7, -1,8, 1,8, 2,7, 2,3, MOVE, 2,8, 2,8, END };
static const int8_t glyph_v[] = { -2,3, 0,8, 2,3, END };
static const int8_t glyph_w[] = { -2,3, -1,8, 0,5, 1,8, 2,3, END };
static const int8_t glyph_x[] = { -2,3, 2,8, MOVE, 2,3, -2,8, END };
static const int8_t glyph_y[] = { -2,3, 0,8, MOVE, 2,3, -1,10, END };
static const int8_t glyph_z[] = { -2,3, 2,3, -2,8, 2,8, END };

static const int8_t glyph_lbrace[] = { 1,0, 0,0, 0,3, -1,4, 0,5, 0,8, 1,8, END };
static const int8_t glyph_pipe[] = { 0,0, 0,8, END };
static const int8_t glyph_rbrace[] = { -1,0, 0,0, 0,3, 1,4, 0,5, 0,8, -1,8, END };
static const int8_t glyph_tilde[] = { -2,4, -1,3, 1,5, 2,4, END };

static const int8_t* glyphs[96] = {
    glyph_space, glyph_excl, glyph_quote, glyph_hash, glyph_dollar, glyph_percent,
    glyph_amp, glyph_apos, glyph_lparen, glyph_rparen, glyph_star, glyph_plus,
    glyph_comma, glyph_minus, glyph_period, glyph_slash,
    glyph_0, glyph_1, glyph_2, glyph_3, glyph_4, glyph_5, glyph_6, glyph_7,
    glyph_8, glyph_9, glyph_colon, glyph_semi, glyph_less, glyph_equal,
    glyph_greater, glyph_quest, glyph_at,
    glyph_A, glyph_B, glyph_C, glyph_D, glyph_E, glyph_F, glyph_G, glyph_H,
    glyph_I, glyph_J, glyph_K, glyph_L, glyph_M, glyph_N, glyph_O, glyph_P,
    glyph_Q, glyph_R, glyph_S, glyph_T, glyph_U, glyph_V, glyph_W, glyph_X,
    glyph_Y, glyph_Z,
    glyph_lbracket, glyph_backslash, glyph_rbracket, glyph_caret, glyph_under, glyph_grave,
    glyph_a, glyph_b, glyph_c, glyph_d, glyph_e, glyph_f, glyph_g, glyph_h,
    glyph_i, glyph_j, glyph_k, glyph_l, glyph_m, glyph_n, glyph_o, glyph_p,
    glyph_q, glyph_r, glyph_s, glyph_t, glyph_u, glyph_v, glyph_w, glyph_x,
    glyph_y, glyph_z,
    glyph_lbrace, glyph_pipe, glyph_rbrace, glyph_tilde, glyph_space
};

#define GLYPH_WIDTH 6.0f
#define GLYPH_HEIGHT 12.0f

void debug_font_draw_char(float x, float y, char c, float scale, color_t color) {
    if (c < 32 || c > 126) c = ' ';
    const int8_t* g = glyphs[c - 32];

    float px = 0, py = 0;
    int have_prev = 0;

    while (*g != END) {
        if (*g == MOVE) {
            have_prev = 0;
            g++;
            continue;
        }
        float cx = x + (float)g[0] * scale;
        float cy = y + (float)g[1] * scale;
        if (have_prev) {
            platform_debug_draw_line(px, py, cx, cy, color);
        }
        px = cx;
        py = cy;
        have_prev = 1;
        g += 2;
    }
}

void debug_font_draw_string(float x, float y, const char* str, float scale, color_t color) {
    while (*str) {
        debug_font_draw_char(x, y, *str, scale, color);
        x += GLYPH_WIDTH * scale;
        str++;
    }
}

void debug_font_draw_float(float x, float y, float value, int precision, float scale, color_t color) {
    char buf[32];
    int neg = value < 0;
    if (neg) value = -value;

    int whole = (int)value;
    float frac = value - (float)whole;

    int i = 0;
    if (neg) buf[i++] = '-';

    int digits[16];
    int nd = 0;
    if (whole == 0) {
        digits[nd++] = 0;
    } else {
        while (whole > 0) {
            digits[nd++] = whole % 10;
            whole /= 10;
        }
    }
    for (int j = nd - 1; j >= 0; j--) {
        buf[i++] = (char)('0' + digits[j]);
    }

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

    debug_font_draw_string(x, y, buf, scale, color);
}

