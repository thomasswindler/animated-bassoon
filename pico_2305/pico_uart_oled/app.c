#include "app.h"
#include "text_ui.h"
#include "hardware/uart.h"
#include "config.h"

void show_and_print(const char *msg) {
    textui_clear();
    textui_write_row0(msg);
    textui_render();

    for (const char *p = msg; *p; ++p)
        uart_putc(UART_ID, *p);
    uart_putc(UART_ID, '\r');
    uart_putc(UART_ID, '\n');
}
