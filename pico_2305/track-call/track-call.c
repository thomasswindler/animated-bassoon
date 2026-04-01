#include <stdio.h>
#include "pico/stdlib.h"

#define TC_PIN
#define RESET_PIN
#define RED_PIN
#define YELLOW_PIN
#define GREEN_PIN
// Maybe add in WS2812 insted of separate LEDs
#define RELAY_PIN

void TCinit()
{
    // Initialize input pins
    gpio_init(TC_PIN);
    gpio_set_dir(TC_PIN, GPIO_IN);
    gpio_pull_up(TC_PIN);

    gpio_init(RESET_PIN);
    gpio_set_dir(RESET_PIN, GPIO_IN);
    gpio_pull_up(RESET_PIN);

    // Initialize output pins
    gpio_init(RED_PIN);
    gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_put(RED_PIN, 0);

    gpio_init(YELLOW_PIN);
    gpio_set_dir(YELLOW_PIN, GPIO_OUT);
    gpio_put(YELLOW_PIN, 0);

    gpio_init(GREEN_PIN);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_put(GREEN_PIN, 0);

    gpio_init(RELAY_PIN);
    gpio_set_dir(RELAY_PIN, GPIO_OUT);
    gpio_put(RELAY_PIN, 0);
}


int main()
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
            break;

        case 1:
            gpio_put(RED_PIN, 0);
            gpio_put(YELLOW_PIN, 1);
            gpio_put(GREEN_PIN, 0);

            state++;
            break;

        case 2:
            gpio_put(RED_PIN, 0);
            gpio_put(YELLOW_PIN, 0);
            gpio_put(GREEN_PIN, 1);

            state++;
            break;

        default:
            gpio_put(RED_PIN, 0);
            gpio_put(YELLOW_PIN, 0);
            gpio_put(GREEN_PIN, 0);

            state = 0;
            break;
        }

        

        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
