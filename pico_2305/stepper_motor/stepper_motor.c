#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define POT_GPIO 26 // Potentiometer GPIO pin
#define SWITCH_GPIO 16 //Dip switch GPIO pin

typedef struct
{
    uint gpio;
    uint speed;
    bool forward;
    uint32_t gpiomask;
    uint phase;
} StepperBi;

uint32_t stepTable[8] = (uint32_t[8]){0x8, 0xC, 0x4, 0x6, 0x2, 0x3, 0x1, 0x9};
/*
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
*/
void StepperBiInit(StepperBi *s, uint gpio)
{
    s->gpio = gpio;

    for (int i = 0; i < 4; i++)
    {
        gpio_set_function((s->gpio) + i, GPIO_FUNC_SIO);
        gpio_set_dir((s->gpio) + i, true);
    }
    s->gpiomask = 0x0F << gpio;
    volatile uint32_t mask = stepTable[0] << gpio;
    gpio_put_masked(s->gpiomask, mask);
    s->phase = 0;
    s->speed = 0;
    s->forward = true;
}

void setPhase(StepperBi *s, uint p)
{
    uint32_t mask = stepTable[p] << (s->gpio);
    gpio_put_masked(s->gpiomask, mask);
}

void stepForward(StepperBi *s)
{
    s->phase = (s->phase + 1) % 8;
    setPhase(s, s->phase);
}

void stepReverse(StepperBi *s)
{
    s->phase = (s->phase - 1 + 8) % 8;
    setPhase(s, s->phase);
}

bool step(struct repeating_timer *t)
{
    StepperBi *s = (StepperBi *)(t->user_data);
    if (s->forward)
    {
        stepForward(s);
    }
    else
    {
        stepReverse(s);
    }
    return true;
}
struct repeating_timer timer;

void rotate(StepperBi *s, bool dir, int speed)
{
    cancel_repeating_timer(&timer);
    s->forward = dir;
    if (speed == 0)
    {
        s->speed = 0;
        return;
    }
    s->speed = 1000 * 1000 / (4 * speed);
    add_repeating_timer_us(s->speed, step, s, &timer);
}

int stepper_test_main()
{
    static StepperBi s1;
    StepperBiInit(&s1, 18);
    rotate(&s1, true, 250);
    while (true)
    {
        rotate(&s1, true, 250);
        sleep_ms(100);
        rotate(&s1, true, 00);
        sleep_ms(100);
    }
    return 0;
}

int main()
{
    stdio_init_all();
    adc_init();
    adc_gpio_init(POT_GPIO); // Initialize the ADC pin for input
    adc_select_input(0); // GPIO26 is ADC input 0

    static StepperBi s1;
    StepperBiInit(&s1, 18);

    gpio_init(SWITCH_GPIO);
    gpio_set_dir(SWITCH_GPIO, GPIO_IN);
    gpio_pull_up(SWITCH_GPIO);

    int print_counter = 0;
    static int prev_speed = -1;
    static bool prev_dir = false;

    while (true)
    {
        uint16_t adc_val = adc_read();
        int speed = (adc_val * 300) / 4095; // Map ADC value (0-4095) to speed (0-500)
        bool dir = gpio_get(SWITCH_GPIO); // DIP switch: on (low) = forward, off (high) = reverse

        if (speed != prev_speed || dir != prev_dir)
        {
            rotate(&s1, dir, speed);
            prev_speed = speed;
            prev_dir = dir;
        }

        print_counter++;
        if (print_counter >= 5)
        {
            printf("ADC: %d, Hex: 0x%03X, Direction: %s, Speed: %d\n", adc_val, adc_val, dir ? "Forward" : "Reverse", speed);
            print_counter = 0;
        }

        sleep_ms(100);
    }

    return 0;
}
