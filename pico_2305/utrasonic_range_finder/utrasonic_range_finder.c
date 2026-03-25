#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/time.h"


#define TRIGGER_PIN 3
#define ECHO_PIN 2


void ultra() {
    gpio_put(TRIGGER_PIN, 0);
    sleep_us(2);
    gpio_put(TRIGGER_PIN, 1);
    sleep_us(10);
    gpio_put(TRIGGER_PIN, 0);
    sleep_us(2);

    while (gpio_get(ECHO_PIN) == 0);
    uint64_t signaloff = time_us_64();
    while (gpio_get(ECHO_PIN) == 1);
    uint64_t signalon = time_us_64();

    uint64_t timepassed = signalon - signaloff;
    // float distance = (timepassed * 0.0343) / 2;
    float distance = ((timepassed * 100) / 5882);
    printf("The distance from object is %.2f cm\n", distance);
}

int main() {
    stdio_init_all();

    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    while (true) {
        ultra();
        sleep_ms(100);
    }

    return 0;
}


/*
volatile uint64_t rise_time = 0;
volatile uint64_t fall_time = 0;
volatile bool echo_received = false;

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == ECHO_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            rise_time = time_us_64();
        } else if (events & GPIO_IRQ_EDGE_FALL) {
            fall_time = time_us_64();
            echo_received = true;
        }
    }
}

void ultra() {
    echo_received = false;
    
    gpio_put(TRIGGER_PIN, 0);
    busy_wait_us(2);
    gpio_put(TRIGGER_PIN, 1);
    busy_wait_us(10);
    gpio_put(TRIGGER_PIN, 0);
    busy_wait_us(2);

    // Wait for echo pulse to complete
    while (!echo_received);

    uint64_t timepassed = fall_time - rise_time;
    float distance = (timepassed * 0.0343) / 2;
    printf("The distance from object is %.2f cm\n", distance);
}

int main() {
    stdio_init_all();

    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    // Enable GPIO interrupts for echo pin
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    while (true) {
        ultra();
        sleep_ms(1000);
    }

    return 0;
}

*/