// Ultrasonic (HC-SR04 style) example for Raspberry Pi Pico (C SDK)
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "MECHws2812.h"

// Constants
#define SOUND_SPEED_MS 340.0f
#define TRIG_PIN 15
#define ECHO_PIN 14
#define TRIG_PULSE_US 10
#define TIMEOUT_US 30000U

// Measure a pulse on pin 'gpio' waiting for target_level (0 or 1).
// Returns pulse length in microseconds, or 0 if timeout.
static uint32_t time_pulse_us(uint gpio, int target_level, uint32_t timeout_us) {
    uint32_t start = time_us_32();

    // Wait for any previous pulse to end (ensure pin != target_level)
    while (gpio_get(gpio) == target_level) {
        if ((time_us_32() - start) >= timeout_us) return 0;
    }

    // Wait for pulse to start
    start = time_us_32();
    while (gpio_get(gpio) != target_level) {
        if ((time_us_32() - start) >= timeout_us) return 0;
    }

    // Measure pulse
    uint32_t t0 = time_us_32();
    while (gpio_get(gpio) == target_level) {
        if ((time_us_32() - t0) >= timeout_us) return 0;
    }
    uint32_t t1 = time_us_32();

    return t1 - t0;
}

static uint32_t color_wheel(uint8_t wheel_pos)
{
    wheel_pos = 255 - wheel_pos;
    if (wheel_pos < 85) // 0..84
        return urgb_u32(255 - wheel_pos * 3, 0, wheel_pos * 3); //multiplies by 3 to scale 0..84 to 0..255
    if (wheel_pos < 170) // 85..169
    {
        wheel_pos -= 85;
        return urgb_u32(0, wheel_pos * 3, 255 - wheel_pos * 3); //
    }
    //if wheel_pos doesn't meet first two if statements
    wheel_pos -= 170; // 170..255
    return urgb_u32(wheel_pos * 3, 255 - wheel_pos * 3, 0);
}

int ultrasonic_main() {
    stdio_init_all();
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_pull_down(ECHO_PIN); // keep echo low when idle

    sleep_ms(100); // let things settle

    while (true) {
        // Ensure idle
        gpio_put(TRIG_PIN, 0);
        sleep_us(5);

        // Send 10 µs pulse
        gpio_put(TRIG_PIN, 1);
        sleep_us(TRIG_PULSE_US);
        gpio_put(TRIG_PIN, 0);

        uint32_t echo_us = time_pulse_us(ECHO_PIN, 1, TIMEOUT_US);
        printf("raw echo_us=%u  ", echo_us);
        if (echo_us == 0) {
            printf("Distance: timeout\n");
        } else {
            float distance_cm = (SOUND_SPEED_MS * (float)echo_us) / 20000.0f;
            printf("Distance: %.2f cm\n", distance_cm);
        }


        sleep_ms(500);
    }

    return 0;
}

int main() {
    stdio_init_all();
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);

    // Initialize WS2812 LEDs
    PIO pio;
    uint sm;
    uint offset;
    
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_pull_down(ECHO_PIN); // keep echo low when idle

    sleep_ms(100);

    while (true) {
        // Ensure idle
        gpio_put(TRIG_PIN, 0);
        sleep_us(5);

        // Send 10 µs pulse
        gpio_put(TRIG_PIN, 1);
        sleep_us(TRIG_PULSE_US);
        gpio_put(TRIG_PIN, 0);

        uint32_t echo_us = time_pulse_us(ECHO_PIN, 1, TIMEOUT_US);
        printf("raw echo_us=%u  ", echo_us);
        if (echo_us == 0) {
            printf("Distance: timeout\n");
        } else {
            float distance_cm = (SOUND_SPEED_MS * (float)echo_us) / 20000.0f;
            // Add in LED output put_pixel(pio, sm, urgb_u32(0, 0, 0));
            uint8_t color_dist = (uint8_t)((float)distance_cm * 255u / 150u);
            put_pixel(pio, sm, color_wheel(color_dist));
            printf("Distance: %.2f cm Color: %u \n", distance_cm, color_dist);
        }


        sleep_ms(500);
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Highly reduced and modified for Mechatrnonics lab by D. W. Frame 2025
 */

inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

inline uint32_t urgbw_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            ((uint32_t) (w) << 24) |
            (uint32_t) (b);
}

int ws2812_main() { //ws2812_main() --- IGNORE ---
    //set_sys_clock_48();
    stdio_init_all();
    printf("WS2812 Smoke Test, using pin %d\n", WS2812_PIN);

    // todo get free sm
    PIO pio;
    uint sm;
    uint offset;

    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
    // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    int t = 0;
    while (true) 
    {
        put_pixel(pio, sm, urgb_u32(0xff, 0, 0));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0, 0xff, 0));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0, 0, 0xff));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0xff));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0xff));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0xff));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0xff));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0xff));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0xff));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        sleep_ms(1000);
        put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0xff));
        sleep_ms(1000);    
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}
