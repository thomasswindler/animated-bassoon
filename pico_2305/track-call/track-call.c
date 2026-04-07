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

// Returns the color for a given state
uint32_t get_state_color(uint state) {
    switch(state) {
        case 0:
            return urgb_u32(0xff, 0, 0); // Green
        case 1:
            return urgb_u32(0x50, 0xff, 0); // Yellow
        case 2:
            return urgb_u32(0, 0xff, 0); // Red
        case 4:
            return urgb_u32(0, 0, 0); // Off
        case 5:
            return urgb_u32(0xff, 0xff, 0xff); // White (for flashing)
        default:
            return urgb_u32(0xff, 0xff, 0xff); // White
    }
}

// Output colors to all 4 LEDs based on their individual states
void track_call_all(PIO pio, uint sm, uint state1, uint state2, uint state3, uint state4) {
    put_pixel(pio, sm, get_state_color(state1)); // LED 1
    put_pixel(pio, sm, get_state_color(state2)); // LED 2
    put_pixel(pio, sm, get_state_color(state3)); // LED 3
    put_pixel(pio, sm, get_state_color(state4)); // LED 4
    sleep_ms(100);
}

// Custom version for flashing that doesn't sleep
void track_call_all_custom(PIO pio, uint sm, uint state1, uint state2, uint state3, uint state4) {
    put_pixel(pio, sm, get_state_color(state1)); // LED 1
    put_pixel(pio, sm, get_state_color(state2)); // LED 2
    put_pixel(pio, sm, get_state_color(state3)); // LED 3
    put_pixel(pio, sm, get_state_color(state4)); // LED 4
}

// Legacy function for backward compatibility
void track_call(PIO pio, uint sm, uint state) {
    put_pixel(pio, sm, get_state_color(state));
    sleep_ms(100);
}

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

    // State for each LED (independently controlled by each TC button)
    uint state1 = 0;  // TC1 controls LED 1
    uint state2 = 0;  // TC2 controls LED 2
    uint state3 = 0;  // TC3 controls LED 3
    uint state4 = 0;  // TC4 controls LED 4

    // Initialize all LEDs to state 0
    track_call_all(pio, sm, state1, state2, state3, state4);

    while (true) {
        if (tc1_pressed) { // TC1 button pressed - controls LED 1
            tc1_pressed = false;
            if (state1 < 2) {
                track_call_all(pio, sm, state1, state2, state3, state4);
                gpio_put(RELAY_PIN, 1);
                state1++;
                sleep_ms(1000);
                gpio_put(RELAY_PIN, 0);
                track_call_all(pio, sm, state1, state2, state3, state4);
            }
            else if (state1 == 2) {
                // 3rd press - show red, then flash white twice and turn off
                track_call_all(pio, sm, state1, state2, state3, state4); // show red
                gpio_put(RELAY_PIN, 1);
                sleep_ms(1000);
                gpio_put(RELAY_PIN, 0);
                
                // Flash white twice with 200ms delay and turn off
                uint temp_state1 = 4; // off
                uint temp_state2 = (state2 >= 3 && state2 != 4) ? 5 : state2;
                uint temp_state3 = (state3 >= 3 && state3 != 4) ? 5 : state3;
                uint temp_state4 = (state4 >= 3 && state4 != 4) ? 5 : state4;
                
                // First flash: off -> white -> off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(200);
                temp_state1 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(200);
                temp_state1 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(200);
                
                // Second flash: off -> white -> off
                temp_state1 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(200);
                temp_state1 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                
                state1 = 4; // Set to "off" state
            }
            else {
                // Flash white once
                // Create temporary states for flashing
                uint temp_state1 = 4; // off
                uint temp_state2 = (state2 >= 3 && state2 != 4) ? 5 : state2;
                uint temp_state3 = (state3 >= 3 && state3 != 4) ? 5 : state3;
                uint temp_state4 = (state4 >= 3 && state4 != 4) ? 5 : state4;
                
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(100);
                temp_state1 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(500);
                temp_state1 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
            }
        }

        if (tc2_pressed) { // TC2 button pressed - controls LED 2
            tc2_pressed = false;
            if (state2 < 2) {
                track_call_all(pio, sm, state1, state2, state3, state4);
                gpio_put(RELAY_PIN, 1);
                state2++;
                sleep_ms(1000);
                gpio_put(RELAY_PIN, 0);
                track_call_all(pio, sm, state1, state2, state3, state4);
            }
            else if (state2 == 2) {
                // 3rd press - show red, then flash white twice and turn off
                track_call_all(pio, sm, state1, state2, state3, state4); // show red
                gpio_put(RELAY_PIN, 1);
                sleep_ms(1000);
                gpio_put(RELAY_PIN, 0);
                
                // Flash white twice with 200ms delay and turn off
                uint temp_state1 = (state1 >= 3 && state1 != 4) ? 5 : state1;
                uint temp_state2 = 4; // off
                uint temp_state3 = (state3 >= 3 && state3 != 4) ? 5 : state3;
                uint temp_state4 = (state4 >= 3 && state4 != 4) ? 5 : state4;
                
                // First flash: off -> white -> off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(200);
                temp_state2 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(200);
                temp_state2 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(200);
                
                // Second flash: off -> white -> off
                temp_state2 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(200);
                temp_state2 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                
                state2 = 4; // Set to "off" state
            }
            else {
                // Flash white once
                uint temp_state1 = (state1 >= 3 && state1 != 4) ? 5 : state1;
                uint temp_state2 = 4; // off
                uint temp_state3 = (state3 >= 3 && state3 != 4) ? 5 : state3;
                uint temp_state4 = (state4 >= 3 && state4 != 4) ? 5 : state4;
                
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(100);
                temp_state2 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(500);
                temp_state2 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
            }
        }

        if (tc3_pressed) { // TC3 button pressed - controls LED 3
            tc3_pressed = false;
            if (state3 < 2) {
                track_call_all(pio, sm, state1, state2, state3, state4);
                gpio_put(RELAY_PIN, 1);
                state3++;
                sleep_ms(1000);
                gpio_put(RELAY_PIN, 0);
                track_call_all(pio, sm, state1, state2, state3, state4);
            }
            else if (state3 == 2) {
                // 3rd press - show red, then flash white twice and turn off
                track_call_all(pio, sm, state1, state2, state3, state4); // show red
                gpio_put(RELAY_PIN, 1);
                sleep_ms(1000);
                gpio_put(RELAY_PIN, 0);
                
                // Flash white twice with 200ms delay and turn off
                uint temp_state1 = (state1 >= 3 && state1 != 4) ? 5 : state1;
                uint temp_state2 = (state2 >= 3 && state2 != 4) ? 5 : state2;
                uint temp_state3 = 4; // off
                uint temp_state4 = (state4 >= 3 && state4 != 4) ? 5 : state4;
                
                // First flash: off -> white -> off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(200);
                temp_state3 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(200);
                temp_state3 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(200);
                
                // Second flash: off -> white -> off
                temp_state3 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(200);
                temp_state3 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                
                state3 = 4; // Set to "off" state
            }
            else {
                // Flash white once
                uint temp_state1 = (state1 >= 3 && state1 != 4) ? 5 : state1;
                uint temp_state2 = (state2 >= 3 && state2 != 4) ? 5 : state2;
                uint temp_state3 = 4; // off
                uint temp_state4 = (state4 >= 3 && state4 != 4) ? 5 : state4;
                
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(100);
                temp_state3 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(500);
                temp_state3 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
            }
        }

        if (tc4_pressed) { // TC4 button pressed - controls LED 4
            tc4_pressed = false;
            if (state4 < 2) {
                track_call_all(pio, sm, state1, state2, state3, state4);
                gpio_put(RELAY_PIN, 1);
                state4++;
                sleep_ms(1000);
                gpio_put(RELAY_PIN, 0);
                track_call_all(pio, sm, state1, state2, state3, state4);
            }
            else if (state4 == 2) {
                // 3rd press - show red, then flash white twice and turn off
                track_call_all(pio, sm, state1, state2, state3, state4); // show red
                gpio_put(RELAY_PIN, 1);
                sleep_ms(1000);
                gpio_put(RELAY_PIN, 0);
                
                // Flash white twice with 200ms delay and turn off
                uint temp_state1 = (state1 >= 3 && state1 != 4) ? 5 : state1;
                uint temp_state2 = (state2 >= 3 && state2 != 4) ? 5 : state2;
                uint temp_state3 = (state3 >= 3 && state3 != 4) ? 5 : state3;
                uint temp_state4 = 4; // off
                
                // First flash: off -> white -> off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(200);
                temp_state4 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(200);
                temp_state4 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(200);
                
                // Second flash: off -> white -> off
                temp_state4 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(200);
                temp_state4 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                
                state4 = 4; // Set to "off" state
            }
            else {
                // Flash white once
                uint temp_state1 = (state1 >= 3 && state1 != 4) ? 5 : state1;
                uint temp_state2 = (state2 >= 3 && state2 != 4) ? 5 : state2;
                uint temp_state3 = (state3 >= 3 && state3 != 4) ? 5 : state3;
                uint temp_state4 = 4; // off
                
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
                sleep_ms(100);
                temp_state4 = 5; // white
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // white
                sleep_ms(500);
                temp_state4 = 4; // off
                track_call_all_custom(pio, sm, temp_state1, temp_state2, temp_state3, temp_state4); // off
            }
        }

        if (reset_pressed) {
            reset_pressed = false;
            // Handle RESET button press - reset all LEDs to state 0
            state1 = 0;
            state2 = 0;
            state3 = 0;
            state4 = 0;
            
            // Animation on reset - cycle all LEDs from red to yellow to green
            // Red
            put_pixel(pio, sm, urgb_u32(0, 0xff, 0));
            put_pixel(pio, sm, urgb_u32(0, 0xff, 0));
            put_pixel(pio, sm, urgb_u32(0, 0xff, 0));
            put_pixel(pio, sm, urgb_u32(0, 0xff, 0));
            sleep_ms(250);
            
            // Yellow
            put_pixel(pio, sm, urgb_u32(0x50, 0xff, 0));
            put_pixel(pio, sm, urgb_u32(0x50, 0xff, 0));
            put_pixel(pio, sm, urgb_u32(0x50, 0xff, 0));
            put_pixel(pio, sm, urgb_u32(0x50, 0xff, 0));
            sleep_ms(250);
            
            // Green
            put_pixel(pio, sm, urgb_u32(0xff, 0, 0));
            put_pixel(pio, sm, urgb_u32(0xff, 0, 0));
            put_pixel(pio, sm, urgb_u32(0xff, 0, 0));
            put_pixel(pio, sm, urgb_u32(0xff, 0, 0));
            sleep_ms(250);
            
            track_call_all(pio, sm, state1, state2, state3, state4);
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

int test_4_leds_main() { //test_4_leds_
    stdio_init_all();
    printf("WS2812 4-LED Test\n");
    printf("Testing 4 separate WS2812 LEDs with different colors\n\n");

    // Initialize WS2812 LEDs
    PIO pio;
    uint sm;
    uint offset;

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Test pattern: cycle through colors on each LED
    while (true) {
        // Pattern 1: Each LED a different color
        printf("Pattern 1: Individual colors (Red, Green, Blue, White)\n");
        put_pixel(pio, sm, urgb_u32(0xff, 0, 0));    // LED 1: Red
        put_pixel(pio, sm, urgb_u32(0, 0xff, 0));    // LED 2: Green
        put_pixel(pio, sm, urgb_u32(0, 0, 0xff));    // LED 3: Blue
        put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0xff)); // LED 4: White
        sleep_ms(2000);

        // Pattern 2: Rainbow-like sequence
        printf("Pattern 2: Rainbow sequence\n");
        put_pixel(pio, sm, urgb_u32(0xff, 0, 0));    // Red
        put_pixel(pio, sm, urgb_u32(0xff, 0xff, 0)); // Yellow
        put_pixel(pio, sm, urgb_u32(0, 0xff, 0));    // Green
        put_pixel(pio, sm, urgb_u32(0, 0xff, 0xff)); // Cyan
        sleep_ms(2000);

        // Pattern 3: Each LED lights individually, then all on
        printf("Pattern 3: Sequential individual LED test\n");
        for (int led = 0; led < 4; led++) {
            // Turn off all LEDs
            for (int i = 0; i < 4; i++) {
                put_pixel(pio, sm, urgb_u32(0, 0, 0));
            }
            sleep_ms(500);

            // Turn on one LED at a time (red)
            for (int i = 0; i < 4; i++) {
                if (i == led) {
                    put_pixel(pio, sm, urgb_u32(0xff, 0, 0)); // Red
                } else {
                    put_pixel(pio, sm, urgb_u32(0, 0, 0));    // Off
                }
            }
            sleep_ms(500);
        }

        // Pattern 4: Brightness test - different intensities of green
        printf("Pattern 4: Brightness levels (Green)\n");
        put_pixel(pio, sm, urgb_u32(0x20, 0, 0));    // LED 1: Dim green
        put_pixel(pio, sm, urgb_u32(0x80, 0, 0));    // LED 2: Medium green
        put_pixel(pio, sm, urgb_u32(0xC0, 0, 0));    // LED 3: Bright green
        put_pixel(pio, sm, urgb_u32(0xff, 0, 0));    // LED 4: Full green
        sleep_ms(2000);

        // Pattern 5: All off
        printf("Pattern 5: All LEDs off\n");
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
        sleep_ms(1000);
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}
