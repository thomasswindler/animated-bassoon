#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "config.h"   // Project configuration: pins, baud rate, I2C params, SSD1306 geometry, button debounce
#include "ssd1306.h"  // SSD1306 display driver API: init/update/clear + framebuffer accessors
#include "text_ui.h"  // Text UI layer: text buffer, putc/newline/backspace handling, render text to SSD1306
#include "app.h"      // App-level helper(s): show_and_print() (display message + also print it over UART)

static const char *BUTTON_MSG = "Hello there";

int main(void) {
    stdio_init_all();

    // UART
    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // I2C
    i2c_init(I2C_PORT, I2C_CLK_KHZ * 1000);

#if defined(PICO_DEFAULT_I2C_SDA_PIN)
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
#endif

    // Button
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Display/UI
    ssd1306_init();
    textui_init();
    ssd1306_clear();

    textui_putc('>');
    textui_render();

    // --- Button Logic Variables ---
    bool last_btn_state = true; // true = not pressed (pull-up)
    absolute_time_t press_start_time = nil_time;
    bool is_waiting_for_release = false; // Tracks if we are in a "wait for release" state
    const int LONG_PRESS_MS = 1000;

    while (true) {
        bool cur = gpio_get(BUTTON_PIN);

        // 1. Detect State Change (Edge Detection)
        if (cur != last_btn_state) {
            // State Changed
            
            if (!cur) { 
                // Button JUST Pressed (Transition: High -> Low)
                press_start_time = get_absolute_time();
                is_waiting_for_release = false;
            } 
            else { 
                // Button JUST Released (Transition: Low -> High)
                
                // Calculate how long it was held
                uint64_t duration_us = absolute_time_diff_us(press_start_time, get_absolute_time());
                int duration_ms = (int)(duration_us / 1000);

                if (duration_ms >= LONG_PRESS_MS) {
                    // It was a Long Press (already handled in the loop below, but just in case)
                    // We rely on the 'is_waiting_for_release' flag to prevent double action
                } else {
                    // It was a Short Press!
                    // Perform Short Press Action
                    for (const char *p = BUTTON_MSG; *p; ++p) {
                        textui_putc(*p);
                        uart_putc(UART_ID, *p);
                    }
                    printf("%s\n", BUTTON_MSG);
                    textui_putc('\n');
                    uart_putc(UART_ID, '\r');
                    uart_putc(UART_ID, '\n');
                    textui_render();
                }
                
                // Reset state
                is_waiting_for_release = false;
            }
            last_btn_state = cur;
            sleep_ms(BUTTON_DEBOUNCE_MS); // Debounce delay after state change
        }

        // 2. Handle Pressed State (While Button is Held)
        if (!cur && !is_waiting_for_release) {
            // Button is currently held down
            uint64_t duration_us = absolute_time_diff_us(press_start_time, get_absolute_time());
            int duration_ms = (int)(duration_us / 1000);

            if (duration_ms >= LONG_PRESS_MS) {
                // LONG PRESS TRIGGERED
                show_and_print("Clearing"); // Clear display
                sleep_ms(500); // Brief pause to show message
                show_and_print(0); // Clear message (and display)
                is_waiting_for_release = true; // Enter "Wait for Release" mode
                
                // Wait for the button to be released
                while (!gpio_get(BUTTON_PIN)) {
                    tight_loop_contents();
                }
                sleep_ms(50); // Debounce release
                // Loop continues, 'cur' will be read as High next iteration
            }
        }

        // 3. UART Echo
        if (uart_is_readable(UART_ID)) {
            int ch = uart_getc(UART_ID);
            if (ch >= 0) {
                uart_putc(UART_ID, (char)ch);
                textui_putc((char)ch);
                textui_render();
            }
        } else {
            tight_loop_contents();
        }
    }

    return 0;
}
