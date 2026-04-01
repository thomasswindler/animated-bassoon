#include <string.h>
#include <stdint.h>
#include "text_ui.h"
#include "ssd1306.h"
#include "config.h"
#include "ssd1306_font.h"

#define SSD1306_PAGE_HEIGHT 8
#define SSD1306_NUM_PAGES   (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)

#define COLS_TEXT (SSD1306_WIDTH / 8)
#define ROWS_TEXT (SSD1306_NUM_PAGES)

static char textbuf[ROWS_TEXT][COLS_TEXT + 1];
static int cur_row = 0;
static int cur_col = 0;

// --- glyph helpers (private) ---
static const uint8_t *lookup_glyph(char c) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z') return &font[(1 + (c - 'A')) * 8];
    if (c >= '0' && c <= '9') return &font[(1 + 26 + (c - '0')) * 8];
    return &font[0];
}

static void draw_glyph_at(int x, int y_page, const uint8_t *glyph) {
    if (y_page < 0 || y_page >= SSD1306_NUM_PAGES) return;
    if (x < 0 || x + 7 >= SSD1306_WIDTH) return;

    uint8_t *fb = ssd1306_fb();
    int base = y_page * SSD1306_WIDTH + x;
    for (int col = 0; col < 8; ++col) fb[base + col] = glyph[col];
}

static void textbuf_scroll(void) {
    for (int r = 0; r < ROWS_TEXT - 1; ++r)
        memcpy(textbuf[r], textbuf[r + 1], COLS_TEXT);

    for (int c = 0; c < COLS_TEXT; ++c) textbuf[ROWS_TEXT - 1][c] = ' ';
    cur_row = ROWS_TEXT - 1;
    cur_col = 0;
}

// --- public API ---
void textui_init(void) {
    for (int r = 0; r < ROWS_TEXT; ++r) {
        for (int c = 0; c < COLS_TEXT; ++c) textbuf[r][c] = ' ';
        textbuf[r][COLS_TEXT] = 0;
    }
    cur_row = 0;
    cur_col = 0;
}

void textui_clear(void) {
    for (int r = 0; r < ROWS_TEXT; ++r)
        for (int c = 0; c < COLS_TEXT; ++c)
            textbuf[r][c] = ' ';
    cur_row = 0;
    cur_col = 0;
}

void textui_write_row0(const char *msg) {
    for (int c = 0; c < COLS_TEXT; ++c) textbuf[0][c] = ' ';
    int x = 0;
    for (const char *p = msg; *p && x < COLS_TEXT; ++p, ++x) textbuf[0][x] = *p;
}

void textui_putc(char ch) {
    if (ch == '\r') return;

    if (ch == '\n') {
        cur_row++;
        cur_col = 0;
        if (cur_row >= ROWS_TEXT) textbuf_scroll();
        return;
    }

    if (ch == '\b' || ch == 127) {
        if (cur_col > 0) {
            cur_col--;
            textbuf[cur_row][cur_col] = ' ';
        } else if (cur_row > 0) {
            cur_row--;
            int last = COLS_TEXT - 1;
            while (last >= 0 && textbuf[cur_row][last] == ' ') last--;
            cur_col = (last < 0) ? 0 : last + 1;
            if (cur_col >= COLS_TEXT) cur_col = COLS_TEXT - 1;

            if (textbuf[cur_row][cur_col] == ' ' && cur_col > 0) cur_col--;
            textbuf[cur_row][cur_col] = ' ';
        }
        return;
    }

    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    if (ch < '0' || (ch > '9' && (ch < 'A' || ch > 'Z'))) ch = 0;

    textbuf[cur_row][cur_col] = ch;
    cur_col++;
    if (cur_col >= COLS_TEXT) {
        cur_col = 0;
        cur_row++;
        if (cur_row >= ROWS_TEXT) textbuf_scroll();
    }
}

void textui_render(void) {
    uint8_t *fb = ssd1306_fb();
    memset(fb, 0, (size_t)ssd1306_fb_len());

    for (int r = 0; r < ROWS_TEXT; ++r) {
        int page = r;
        for (int c = 0; c < COLS_TEXT; ++c) {
            const uint8_t *g = lookup_glyph(textbuf[r][c]);
            draw_glyph_at(c * 8, page, g);
        }
    }
    ssd1306_update();
}
