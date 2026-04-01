#pragma once

// --- Display geometry ---
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT 32
#endif
#define SSD1306_WIDTH 128

// --- UART ---
#define UART_ID        uart0
#define UART_BAUD      115200
#define UART_TX_PIN    0
#define UART_RX_PIN    1

// --- I2C / SSD1306 ---
#define I2C_PORT          i2c_default
#define SSD1306_I2C_ADDR  0x3C
#define I2C_CLK_KHZ       400

// --- Button ---
#define BUTTON_PIN          14
#define BUTTON_DEBOUNCE_MS  10
