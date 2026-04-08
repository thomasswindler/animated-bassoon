#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>

int main() {
    stdio_init_all();

    spi_init(spi0, 1 * 1000 * 1000);

    gpio_set_function(16, GPIO_FUNC_SPI);
    gpio_set_function(19, GPIO_FUNC_SPI);
    gpio_set_function(18, GPIO_FUNC_SPI);
    // gpio_set_function(17, GPIO_FUNC_SPI); // Remove for manual CS
    gpio_init(17);
    gpio_set_dir(17, GPIO_OUT);
    gpio_put(17, 1); // CS high (deselect)

    uint8_t tx = 0x01;
    uint8_t rx;

    while (true) {
        gpio_put(17, 0); // CS low (select slave)
        spi_write_read_blocking(spi0, &tx, &rx, 1);
        gpio_put(17, 1); // CS high (deselect)
        printf("Sent: %02X | Received: %02X\n", tx, rx);
        tx++;
        sleep_ms(1000);
    }
}
