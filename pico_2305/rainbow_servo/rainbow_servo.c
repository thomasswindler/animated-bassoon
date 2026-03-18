#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "MECHws2812.h"

//will need to use pin 28 for potentiometer input since it has an ADC
#define ADC_GPIO_PIN 26 //The SDK will need the ADC pin to be defined as 0 since the ADC pin is ADC0 on GPIO26
#define ADC_PIN 0 //See the SDK ADC section for finding the ADC pin number
#define SERVO_PIN 18
//LED pin is defined in the MECHws2812.h file

#ifdef PICO_DEFAULT_LED_PIN
#define LED_PIN PICO_DEFAULT_LED_PIN
#endif


uint32_t pwm_set_freq_duty(uint slice_num, uint chan, uint32_t f, int d)
{
    uint32_t clock = 125000000;
    uint32_t divider16 = clock / f / 4096 + (clock % (f * 4096) != 0);
    if (divider16 / 16 == 0)
        divider16 = 16;
    uint32_t wrap = clock * 16 / divider16 / f - 1;
    pwm_set_clkdiv_int_frac(slice_num, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap * d / 100);
    return wrap;
}

uint32_t pwm_get_wrap(uint slice_num)
{
    // valid_params_if(PWM, slice_num >= 0 && slice_num < NUM_PWM_SLICES);
    return pwm_hw->slice[slice_num].top;
}

void pwm_set_duty(uint slice_num, uint chan, int d)
{
    pwm_set_chan_level(slice_num, chan, pwm_get_wrap(slice_num) * d / 100);
}

void pwm_set_dutyH(uint slice_num, uint chan, int d)
{
    pwm_set_chan_level(slice_num, chan, pwm_get_wrap(slice_num) * d / 10000);
}

typedef struct
{
    uint gpio;
    uint slice;
    uint chan;
    uint speed;
    uint resolution;
    bool on;
    bool invert;
} Servo;

void ServoInit(Servo *s, uint gpio, bool invert)
{
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    s->gpio = gpio;
    s->slice = pwm_gpio_to_slice_num(gpio);
    s->chan = pwm_gpio_to_channel(gpio);

    pwm_set_enabled(s->slice, false);
    s->on = false;
    s->speed = 0;
    s->resolution = pwm_set_freq_duty(s->slice, s->chan, 50, 0);
    pwm_set_dutyH(s->slice, s->chan, 250);
    if (s->chan)
    {
        pwm_set_output_polarity(s->slice, false, invert);
    }
    else
    {
        pwm_set_output_polarity(s->slice, invert, false);
    }
    s->invert = invert;
}

void ServoOn(Servo *s)
{
    pwm_set_enabled(s->slice, true);
    s->on = true;
}

void ServoOff(Servo *s)
{
    pwm_set_enabled(s->slice, false);
    s->on = false;
}
void ServoPosition(Servo *s, uint p)
{
    pwm_set_dutyH(s->slice, s->chan, p * 10 + 250);
}

static uint32_t color_wheel(uint8_t wheel_pos) // Input a value 0 to 255 to get a color value. The colours are a transition r - g - b - back to r.
{
    wheel_pos = 255 - wheel_pos; // Invert wheel position to have red at 0 and green at 85 and blue at 170
    //If statements multiply by 3 because wheel_pos is "divided" into 3 sections by the if statements.
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

int servo_test_main() //code to test the servo connections
{
    Servo s1;
    ServoInit(&s1, SERVO_PIN, false); // Connect the servo signal wire to GPIO 18

    ServoOn(&s1);
    while (true)
    {
        ServoPosition(&s1, 0);
        sleep_ms(1000);
        ServoPosition(&s1, 100);
        sleep_ms(1000);
    }

    return 0;
}

int ADC_Test_main() //code to test the potentiometer connections
{
    stdio_init_all();
    printf("ADC Example, measuring GPIO26\n");

    adc_init();

    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    // Select ADC input 0 (GPIO26)
    adc_select_input(0);

    while (1) {
        // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
        const float conversion_factor = 3.3f / (1 << 12);
        uint16_t result = adc_read();
        printf("Raw value: 0x%03x, voltage: %f V\n", result, result * conversion_factor);
        sleep_ms(500);
    }
}

int servo_position_main() //change servo position based on potentiometer input
{
    stdio_init_all();
    adc_init();

    Servo s1;
    ServoInit(&s1, SERVO_PIN, false); // Connect the servo signal wire SERVO_PIN
    ServoOn(&s1);

    adc_gpio_init(ADC_GPIO_PIN); // Initialize the ADC pin for input
    adc_select_input(ADC_PIN); // GPIO26 is ADC input 0

    while (1)
    {
        uint16_t potentiometer = adc_read(); // 12-bit ADC: 0..4095 (0x000..0xFFF)

        // Map ADC range [0, 4095] to servo position [0, 100]
        uint pos = (uint)((uint32_t)potentiometer * 100u / 4095u);

        ServoPosition(&s1, pos);

        // Optional: print values for debugging over USB serial
        printf("ADC=0x%03x (%u), pos=%u\n", potentiometer, potentiometer, pos);
        sleep_ms(20);
    }
}


int main() //change servo position based on potentiometer input; change LED color based on servo position
{
    stdio_init_all();
    adc_init();

    PIO pio;
    uint sm;
    uint offset;

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    Servo s1;
    ServoInit(&s1, SERVO_PIN, false); // Connect the servo signal wire SERVO_PIN
    ServoOn(&s1);

    adc_gpio_init(ADC_GPIO_PIN); // Initialize the ADC pin for input
    adc_select_input(ADC_PIN); // GPIO26 is ADC input 0

    while (1)
    {
        uint16_t potentiometer = adc_read(); // 12-bit ADC: 0..4095 (0x000..0xFFF)

        // Map ADC range [0, 4095] to servo position [0, 100]
        uint pos = (uint)((uint32_t)potentiometer * 100u / 4095u);

        ServoPosition(&s1, (pos));

        // Map servo position to hue on wheel (0..255)
        uint8_t hue = (uint8_t)((uint32_t)pos * 255u / 100u);
        put_pixel(pio, sm, color_wheel(hue));

        // Optional: print values for debugging over USB serial
        printf("ADC=0x%03x (%u), pos=%u, hue=%u\n", potentiometer, potentiometer, pos, hue);
        sleep_ms(20);
    }

    // never reached, but cleanup if needed
    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}

//--------------------------------------------------------------------//

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
