// Echo UART0 to UART0 and render incoming text on SSD1306 using provided font[].
// Assumes default I2C pins (GP4 SDA, GP5 SCL) and UART0 on GP0/GP1.
// Configure SSD1306_HEIGHT to 32 or 64 as needed.

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "ssd1306_font.h"

#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT 32
#endif
#define SSD1306_WIDTH 128

#define UART_ID        uart0
#define UART_BAUD      115200
#define UART_TX_PIN    0
#define UART_RX_PIN    1

#define I2C_PORT       i2c_default
#define SSD1306_I2C_ADDR 0x3C
#define I2C_CLK_KHZ 400

#define SSD1306_PAGE_HEIGHT 8
#define SSD1306_NUM_PAGES (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define BUTTON_PIN  14   // GPIO Pin used for button
#define BUTTON_DEBOUNCE_MS 50
const char *BUTTON_MSG = "Hello There"; // Message to be displayed when button pressed.

#define COLS_TEXT (SSD1306_WIDTH / 8)
#define ROWS_TEXT (SSD1306_NUM_PAGES)

static char textbuf[ROWS_TEXT][COLS_TEXT + 1];
static void render_textbuf_to_fb(void);


// --- framebuffer ---
static uint8_t fb[SSD1306_BUF_LEN];

// --- SSD1306 helpers ---
static void ssd1306_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, buf, 2, false);
}

static void ssd1306_send_cmds(const uint8_t *cmds, int n) {
    for (int i = 0; i < n; ++i) ssd1306_send_cmd(cmds[i]);
}

static void ssd1306_send_buf(const uint8_t *buf_in, int len) {
    uint8_t *tmp = malloc(len + 1);
    if (!tmp) return;
    tmp[0] = 0x40;
    memcpy(tmp + 1, buf_in, len);
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, tmp, len + 1, false);
    free(tmp);
}

static void ssd1306_init(void) {
    const uint8_t init_seq[] = {
        0xAE,0x20,0x00,0x40,0xA1,0xC8,0xA8,(uint8_t)(SSD1306_HEIGHT-1),
        0xD3,0x00,0xDA,(SSD1306_HEIGHT==64)?0x12:0x02,0x81,0x7F,0xD5,0x80,0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0x8D,0x14,0xAF
    };
    ssd1306_send_cmds(init_seq, sizeof(init_seq));
}

static void ssd1306_update(void) {
    uint8_t cmds[] = {0x21,0x00,SSD1306_WIDTH-1,0x22,0x00,SSD1306_NUM_PAGES-1};
    ssd1306_send_cmds(cmds, sizeof(cmds));
    ssd1306_send_buf(fb, SSD1306_BUF_LEN);
}

static void ssd1306_clear(void) {
    memset(fb,0,SSD1306_BUF_LEN);
    ssd1306_update();
}

static void show_and_print(const char *msg) {
    // clear buffer
    for (int r = 0; r < ROWS_TEXT; ++r)
        for (int c = 0; c < COLS_TEXT; ++c) textbuf[r][c] = ' ';
    // write msg at row 0, bounded
    int x = 0;
    for (const char *p = msg; *p && x < COLS_TEXT; ++p, ++x) textbuf[0][x] = *p;
    render_textbuf_to_fb();
    // send to UART with newline
    for (const char *p = msg; *p; ++p) uart_putc(UART_ID, *p);
    uart_putc(UART_ID, '\r');
    uart_putc(UART_ID, '\n');
}



// --- text rendering using the vertical font ---
// font[] layout: first entry is 'nothing', then A-Z then 0-9.
// We'll map ASCII input to that set: uppercase letters and digits; everything else as space.
// Place vertical 8-byte glyph into framebuffer at pixel column x and page y_page
static void draw_glyph_at(int x, int y_page, const uint8_t *glyph) {
    if (y_page < 0 || y_page >= SSD1306_NUM_PAGES) return;
    if (x < 0 || x + 7 >= SSD1306_WIDTH) return;
    // fb layout: page-major, each page has SSD1306_WIDTH bytes; each byte = vertical 8 pixels for that column
    int base = y_page * SSD1306_WIDTH + x;
    for (int col = 0; col < 8; ++col) {
        fb[base + col] = glyph[col]; // glyph[col] is vertical byte for that column
    }
}

static const uint8_t *lookup_glyph(char c) {
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A'; // map lowercase to uppercase
    if (c >= 'A' && c <= 'Z') return &font[(1 + (c - 'A')) * 8];
    if (c >= '0' && c <= '9') return &font[(1 + 26 + (c - '0')) * 8];
    return &font[0];
}


#define COLS_TEXT (SSD1306_WIDTH / 8)
#define ROWS_TEXT (SSD1306_NUM_PAGES)

static char textbuf[ROWS_TEXT][COLS_TEXT + 1];
static int cur_row = 0;
static int cur_col = 0;

static void textbuf_init(void) {
    for (int r = 0; r < ROWS_TEXT; ++r)
        for (int c = 0; c < COLS_TEXT; ++c) textbuf[r][c] = ' ';
    for (int r = 0; r < ROWS_TEXT; ++r) textbuf[r][COLS_TEXT] = 0;
    cur_row = cur_col = 0;
}

static void textbuf_scroll(void) {
    for (int r = 0; r < ROWS_TEXT - 1; ++r) memcpy(textbuf[r], textbuf[r+1], COLS_TEXT);
    for (int c = 0; c < COLS_TEXT; ++c) textbuf[ROWS_TEXT-1][c] = ' ';
    cur_row = ROWS_TEXT-1; cur_col = 0;
}

static void textbuf_putc(char ch) {
    if (ch == '\r') return;
    if (ch == '\n') { cur_row++; cur_col = 0; if (cur_row >= ROWS_TEXT) textbuf_scroll(); return; }

    if (ch == '\b' || ch == 127) {
        if (cur_col > 0) {
            cur_col--;
            textbuf[cur_row][cur_col] = ' ';
        } else if (cur_row > 0) {
            // move to previous row end (last occupied column)
            cur_row--;
            // find last non-space in previous row
            int last = COLS_TEXT - 1;
            while (last >= 0 && textbuf[cur_row][last] == ' ') last--;
            // place caret after last char (or at start if empty)
            cur_col = (last < 0) ? 0 : last + 1;
            if (cur_col >= COLS_TEXT) cur_col = COLS_TEXT - 1;
            // delete previous char if exists
            if (cur_col > 0 || textbuf[cur_row][cur_col] != ' ') {
                if (textbuf[cur_row][cur_col] == ' ') cur_col--;
                if (cur_col < 0) cur_col = 0;
                textbuf[cur_row][cur_col] = ' ';
            }
        }
        return;
    }

    // map lowercase to uppercase for font lookup
    if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';

    // accept A-Z and 0-9 only, others => nothing
    if (ch < '0' || (ch > '9' && (ch < 'A' || ch > 'Z'))) ch = 0;

    textbuf[cur_row][cur_col] = ch;
    cur_col++;
    if (cur_col >= COLS_TEXT) { cur_col = 0; cur_row++; if (cur_row >= ROWS_TEXT) textbuf_scroll(); }
}


static void render_textbuf_to_fb(void) {
    memset(fb, 0, SSD1306_BUF_LEN);
    for (int r = 0; r < ROWS_TEXT; ++r) {
        int page = r; // one text row per page
        for (int c = 0; c < COLS_TEXT; ++c) {
            const uint8_t *g = lookup_glyph(textbuf[r][c]);
            int x = c * 8;
            draw_glyph_at(x, page, g);
        }
    }
    ssd1306_update();
}

// --- main ---
int main(void) {
    stdio_init_all();

    // UART
    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // I2C
    i2c_init(I2C_PORT, I2C_CLK_KHZ * 1000);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // button to GND when pressed


#if defined(PICO_DEFAULT_I2C_SDA_PIN)
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
#endif

    ssd1306_init();
    textbuf_init();
    ssd1306_clear();

    // prompt
    textbuf_putc('>');
    render_textbuf_to_fb();

bool last_btn_state = true; // not pressed (pull-up)
absolute_time_t last_debounce_time = nil_time;
while (true) {
    // button check
    bool cur = gpio_get(BUTTON_PIN);
    if (cur != last_btn_state) {
        // state changed, start debounce timer
        last_debounce_time = make_timeout_time_ms(BUTTON_DEBOUNCE_MS);
        last_btn_state = cur;
    } else {
        // stable state for debounce interval
        if (!cur && !time_reached(last_debounce_time)) {
            // still unstable, skip
        } else if (!cur && time_reached(last_debounce_time)) {
            // button is pressed (active low) and debounced
            show_and_print(BUTTON_MSG);
            // wait until released to avoid repeats
            while (!gpio_get(BUTTON_PIN)) tight_loop_contents();
            sleep_ms(50);
        }
    }

    // UART handling as before
    if (uart_is_readable(UART_ID)) {
        int ch = uart_getc(UART_ID);
        if (ch >= 0) {
            uart_putc(UART_ID, ch);
            textbuf_putc((char)ch);
            render_textbuf_to_fb();
        }
    } else {
        tight_loop_contents();
    }
    }

    return 0;
}
