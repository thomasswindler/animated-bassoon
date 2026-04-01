// pico_uart_echo.c
// Echo data received on UART0 back to sender.
// Built for Raspberry Pi Pico (RP2040) using the Pico SDK.
// UART0 on GPIO0 (TX) and GPIO1 (RX). Baud rate 115200.

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define UART_ID uart0
#define BAUD_RATE 115200

// Pins for UART0
#define UART_TX_PIN 0  // GP0
#define UART_RX_PIN 1  // GP1

int main() {
    stdio_init_all(); // optional, leaves stdio untouched but safe
    // Initialize UART0
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by function (UART)
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Optional: set FIFO levels / formats
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // Enable RX interrupt (not required for polling echo)
    // uart_set_irq_enables(UART_ID, true, false);

    // Simple echo loop: read bytes and write them back
    while (true) {
        // Poll for available data
        if (uart_is_readable(UART_ID)) {
            uint8_t ch = uart_getc(UART_ID);
            // Echo it back
            uart_putc(UART_ID, ch);
            // Also flush TX FIFO to ensure immediate send
            // (uart_putc is blocking until FIFO has room)
        } else {
            // Small sleep to yield CPU
            tight_loop_contents();
        }
    }

    return 0;
}
