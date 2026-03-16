#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

//will need to use pin 28 for potentiometer input since it has an ADC
#define ADC_PIN 26 //The SDK will need the ADC pin to be defined as 0 since the ADC pin is ADC0 on GPIO26
#define SERVO_PIN 18


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

int servo_test_main()
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

int main()
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
