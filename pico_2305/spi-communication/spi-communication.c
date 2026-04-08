#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>

int main() {
    stdio_init_all();

    spi_init(spi0, 1 * 1000 * 1000);
    spi_set_slave(spi0, true);

    gpio_set_function(16, GPIO_FUNC_SPI); // MISO
    gpio_set_function(19, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(18, GPIO_FUNC_SPI); // SCK
    gpio_set_function(17, GPIO_FUNC_SPI); // CS

    uint8_t rx, tx = 0x55;

    while (true) {
        spi_read_blocking(spi0, tx, &rx, 1);
        printf("Slave: sent %02X, received %02X\n", tx, rx);
        tx = rx; // echo what we received
    }
}