#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"
#include <stdio.h>

#define I2C_PORT i2c0
#define SLAVE_ADDR 0x42

static uint8_t tx_data = 0;

static void on_i2c_slave_event(i2c_inst_t *i2c, i2c_slave_event_t event) {
    if (event == I2C_SLAVE_REQUEST) {
        tx_data++;
        if (i2c_get_write_available(i2c)) {
            i2c_write_raw_blocking(i2c, &tx_data, 1);
        }
    }
}

int main() {
    stdio_init_all();

    i2c_init(I2C_PORT, 100 * 1000);

    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    i2c_slave_init(I2C_PORT, SLAVE_ADDR, on_i2c_slave_event);

    while (true) {
        tight_loop_contents();
    }
}
