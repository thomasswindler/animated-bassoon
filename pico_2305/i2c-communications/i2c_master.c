#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>

#define I2C_PORT i2c0
#define SLAVE_ADDR 0x42

int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, 100 * 1000);

    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    uint8_t rx;

    while (true) {
        i2c_read_blocking(I2C_PORT, SLAVE_ADDR, &rx, 1, false);
        printf("Received: %d\n", rx);
        sleep_ms(1000);
    }
}