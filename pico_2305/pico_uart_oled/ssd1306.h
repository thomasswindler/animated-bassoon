#pragma once
#include <stdint.h>
#include "config.h"

void ssd1306_init(void);
void ssd1306_update(void);
void ssd1306_clear(void);

// Framebuffer access for renderers:
uint8_t *ssd1306_fb(void);
int ssd1306_fb_len(void);
