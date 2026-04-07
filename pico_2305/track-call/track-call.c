#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "MECHws2812.h"

#define TC1_PIN 16
#define TC2_PIN 17
#define TC3_PIN 18
#define TC4_PIN 19
#define RESET_PIN 20
// #define RED_PIN
// #define YELLOW_PIN
// #define GREEN_PIN
#define RELAY_PIN 15

volatile bool tc1_pressed = false;
volatile bool tc2_pressed = false;
volatile bool tc3_pressed = false;
volatile bool tc4_pressed = false;
volatile bool reset_pressed = false;
struct repeating_timer tc1_timer;
struct repeating_timer tc2_timer;
struct repeating_timer tc3_timer;
struct repeating_timer tc4_timer;
struct repeating_timer reset_timer;

void gpio_callback(uint gpio, uint32_t events);
bool tc1_debounce_callback(struct repeating_timer *t);
bool tc2_debounce_callback(struct repeating_timer *t);
bool tc3_debounce_callback(struct repeating_timer *t);
bool tc4_debounce_callback(struct repeating_timer *t);
bool reset_debounce_callback(struct repeating_timer *t);

void TCinit()
{
    // Initialize input pins
    gpio_init(TC1_PIN);
    gpio_set_dir(TC1_PIN, GPIO_IN);
    gpio_pull_up(TC1_PIN);

    gpio_init(TC2_PIN);
    gpio_set_dir(TC2_PIN, GPIO_IN);
    gpio_pull_up(TC2_PIN);

    gpio_init(TC3_PIN);
    gpio_set_dir(TC3_PIN, GPIO_IN);
    gpio_pull_up(TC3_PIN);

    gpio_init(TC4_PIN);
    gpio_set_dir(TC4_PIN, GPIO_IN);
    gpio_pull_up(TC4_PIN);

    gpio_init(RESET_PIN);
    gpio_set_dir(RESET_PIN, GPIO_IN);
    gpio_pull_up(RESET_PIN);

    // Set up interrupts for button debounce
    gpio_set_irq_enabled_with_callback(TC1_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(TC2_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(TC3_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(TC4_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(RESET_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Initialize output pins
    /*
    gpio_init(RED_PIN);
    gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_put(RED_PIN, 0);

    gpio_init(YELLOW_PIN);
    gpio_set_dir(YELLOW_PIN, GPIO_OUT);
    gpio_put(YELLOW_PIN, 0);

    gpio_init(GREEN_PIN);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_put(GREEN_PIN, 0);
    */
    gpio_init(RELAY_PIN);
    gpio_set_dir(RELAY_PIN, GPIO_OUT);
    gpio_put(RELAY_PIN, 0);
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == TC1_PIN) {
        add_repeating_timer_ms(50, tc1_debounce_callback, NULL, &tc1_timer);
    } else if (gpio == TC2_PIN) {
        add_repeating_timer_ms(50, tc2_debounce_callback, NULL, &tc2_timer);
    } else if (gpio == TC3_PIN) {
        add_repeating_timer_ms(50, tc3_debounce_callback, NULL, &tc3_timer);
    } else if (gpio == TC4_PIN) {
        add_repeating_timer_ms(50, tc4_debounce_callback, NULL, &tc4_timer);
    } else if (gpio == RESET_PIN) {
        add_repeating_timer_ms(50, reset_debounce_callback, NULL, &reset_timer);
    }
}

bool tc1_debounce_callback(struct repeating_timer *t) {
    if (!gpio_get(TC1_PIN)) { // still low, pressed
        tc1_pressed = true;
    }
    return false; // don't repeat
}

bool tc2_debounce_callback(struct repeating_timer *t) {
    if (!gpio_get(TC2_PIN)) { // still low, pressed
        tc2_pressed = true;
    }
    return false; // don't repeat
}

bool tc3_debounce_callback(struct repeating_timer *t) {
    if (!gpio_get(TC3_PIN)) { // still low, pressed
        tc3_pressed = true;
    }
    return false; // don't repeat
}

bool tc4_debounce_callback(struct repeating_timer *t) {
    if (!gpio_get(TC4_PIN)) { // still low, pressed
        tc4_pressed = true;
    }
    return false; // don't repeat
}

bool reset_debounce_callback(struct repeating_timer *t) {
    if (!gpio_get(RESET_PIN)) { // still low, pressed
        reset_pressed = true;
    }
    return false; // don't repeat
}

void track_call(PIO pio, uint sm, uint state)
{
    switch(state)
    {
        case 0:
            put_pixel(pio, sm, urgb_u32(0xff, 0, 0)); // Green
            sleep_ms(100);
            break;
        case 1:
            put_pixel(pio, sm, urgb_u32(0x50, 0xff, 0)); // Yellow
            sleep_ms(100);
            break;
        case 2:
            put_pixel(pio, sm, urgb_u32(0, 0xff, 0)); // Red
            sleep_ms(100);
            break;
        default: // When case doesn't equal 0, 1, 2 //Flash white once
            put_pixel(pio, sm, urgb_u32(0, 0, 0));
            sleep_ms(100);
            put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0xff));
            sleep_ms(500);
            put_pixel(pio, sm, urgb_u32(0, 0, 0));
            break;
    }
}


/*
int switch_test_main()
{
    int state = 0;
    stdio_init_all();
    TCinit();

    while (true) {
        switch (state)
        {
        case 0:
            gpio_put(RED_PIN, 1);
            gpio_put(YELLOW_PIN, 0);
            gpio_put(GREEN_PIN, 0);

            state++;
            sleep_ms(1000);
            break;

        case 1:
            gpio_put(RED_PIN, 0);
            gpio_put(YELLOW_PIN, 1);
            gpio_put(GREEN_PIN, 0);

            state++;
            sleep_ms(1000);
            break;

        case 2:
            gpio_put(RED_PIN, 0);
            gpio_put(YELLOW_PIN, 0);
            gpio_put(GREEN_PIN, 1);

            state++;
            sleep_ms(1000);
            break;

        default:
            gpio_put(RED_PIN, 0);
            gpio_put(YELLOW_PIN, 0);
            gpio_put(GREEN_PIN, 0);

            state = 0;
            sleep_ms(1000);
            break;
        }

        printf("Hello, world!\n");
    }
}
*/

void main()
{
    stdio_init_all();
    TCinit();

    // Initialize WS2812 LEDs
    PIO pio;
    uint sm;
    uint offset;
    
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    //
    uint state = 0;
    track_call(pio, sm, state); // Initialize to state 0
    while (true) {
        if (tc1_pressed) { // TC1 button pressed
            tc1_pressed = false;
            // Handle TC1 button press
            if (state < 3) {
                track_call(pio, sm, state);
                gpio_put(RELAY_PIN, 1);
                state++;
                sleep_ms(1000);
                gpio_put(RELAY_PIN, 0);
                track_call(pio, sm, state);
            }
            else if (state >= 3) {
                track_call(pio, sm, state); // Flash white
            }
        }

        if (tc2_pressed) { // TC2 button pressed
            tc2_pressed = false;
            // Handle TC2 button press
        }

        if (tc3_pressed) { // TC3 button pressed
            tc3_pressed = false;
            // Handle TC3 button press
        }

        if (tc4_pressed) { // TC4 button pressed
            tc4_pressed = false;
            // Handle TC4 button press
        }

        if (reset_pressed) {
            reset_pressed = false;
            // Handle RESET button press, e.g., reset state
            state = 0;
            put_pixel(pio, sm, urgb_u32(0, 0, 0));
            sleep_ms(100);
            put_pixel(pio, sm, urgb_u32(0, 0xff, 0)); // Red
            sleep_ms(250);
            put_pixel(pio, sm, urgb_u32(0x50, 0xff, 0)); // Yellow
            sleep_ms(250);
            put_pixel(pio, sm, urgb_u32(0xff, 0, 0)); // Green
            sleep_ms(250);
            track_call(pio, sm, state);
        }
    }

}


// ------------------------------------------------------------------- //

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
