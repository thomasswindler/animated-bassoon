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

    bool last_btn_state = true;                // pull-up => true = not pressed
    absolute_time_t debounce_deadline = nil_time;

    while (true) {
        // Button debounce
        bool cur = gpio_get(BUTTON_PIN);
        if (cur != last_btn_state) {
            debounce_deadline = make_timeout_time_ms(BUTTON_DEBOUNCE_MS);
            last_btn_state = cur;
        } else 
            if (!cur && time_reached(debounce_deadline)) {
                for (const char *p = BUTTON_MSG; *p; ++p) {
                    textui_putc(*p);
                    uart_putc(UART_ID, *p);
                }
                printf("%s\n", BUTTON_MSG);
                textui_putc('\n');
                uart_putc(UART_ID, '\r');
                uart_putc(UART_ID, '\n');
                textui_render();
                while (!gpio_get(BUTTON_PIN))
                    tight_loop_contents();
                sleep_ms(50);
            }
            /*
            if (!cur && time_reached(debounce_deadline)) {
            show_and_print(BUTTON_MSG);
            while (!gpio_get(BUTTON_PIN)) tight_loop_contents();
            sleep_ms(50);
            }*/
        

        // UART echo + render
        if (uart_is_readable(UART_ID)) {
            int ch = uart_getc(UART_ID);
            uart_putc(UART_ID, (char)ch);
            textui_putc((char)ch);
            textui_render();
            printf("%c", (char)ch);
        } else {
            tight_loop_contents();
        }
    }
}
