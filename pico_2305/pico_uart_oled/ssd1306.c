#include <string.h>
#include <stdlib.h>
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "config.h"

#define SSD1306_PAGE_HEIGHT 8
#define SSD1306_NUM_PAGES   (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN     (SSD1306_NUM_PAGES * SSD1306_WIDTH)

static uint8_t fb[SSD1306_BUF_LEN];

// --- low-level helpers (private) ---
static void ssd1306_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, buf, 2, false);
}

static void ssd1306_send_cmds(const uint8_t *cmds, int n) {
    for (int i = 0; i < n; ++i) ssd1306_send_cmd(cmds[i]);
}

static void ssd1306_send_buf(const uint8_t *buf_in, int len) {
    uint8_t *tmp = (uint8_t *)malloc((size_t)len + 1);
    if (!tmp) return;
    tmp[0] = 0x40;
    memcpy(tmp + 1, buf_in, (size_t)len);
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, tmp, len + 1, false);
    free(tmp);
}

// --- public API ---
void ssd1306_init(void) {
    const uint8_t init_seq[] = {
        0xAE,0x20,0x00,0x40,0xA1,0xC8,0xA8,(uint8_t)(SSD1306_HEIGHT-1),
        0xD3,0x00,0xDA,(SSD1306_HEIGHT==64)?0x12:0x02,0x81,0x7F,0xD5,0x80,
        0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0x8D,0x14,0xAF
    };
    ssd1306_send_cmds(init_seq, (int)sizeof(init_seq));
}

void ssd1306_update(void) {
    uint8_t cmds[] = {0x21,0x00,SSD1306_WIDTH-1,0x22,0x00,SSD1306_NUM_PAGES-1};
    ssd1306_send_cmds(cmds, (int)sizeof(cmds));
    ssd1306_send_buf(fb, SSD1306_BUF_LEN);
}

void ssd1306_clear(void) {
    memset(fb, 0, SSD1306_BUF_LEN);
    ssd1306_update();
}

uint8_t *ssd1306_fb(void) { return fb; }
int ssd1306_fb_len(void) { return SSD1306_BUF_LEN; }
