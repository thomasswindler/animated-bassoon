/* 
Originally written by Ethan Moyes
Pulled code from Professor David Frame's lectures and Pico SDK
circuit follows 2205 SCR firing by microcontroller assignment
APR2024 Changes (DWF):  Renamed variables and defines to be more clear
                        added many units to variable names
                        Moved ADC init to InitInputOutput procedure
                        Added comments
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define GATE_OUTPIN 2     // goes high after delay when interreupt goes low
#define INTERRUPT_INPIN 4 // Goes low when the AC signal is on the positive cycle
#define ANALOG_INPIN 26   // (A0)connects to 0-3.3V from pot (ADC input 0)
#define ELEMENTS(x) (sizeof(x)/sizeof(x)[0]) //returns number of elements in an array

uint firingDelay_us = 0;

//used to "map" analog signal to time range in us
uint32_t map(uint32_t IN, uint32_t INmin, uint32_t INmax, uint32_t OUTmin, uint32_t OUTmax)  
{
    return ((((IN - INmin)*(OUTmax - OUTmin))/(INmax - INmin)) + OUTmin);
}

void zeroCrossing_callback(uint gpio, uint32_t events)
{
    busy_wait_us(firingDelay_us);
    gpio_put(GATE_OUTPIN, true);
    busy_wait_ms(1); //ensures there are no "re-=triggers" for this alteration
    gpio_put(GATE_OUTPIN, false);
}

// initalizing all digital and analog input and output pins as well as
void InitInputsandOutputs()
    {
        const uint outs[]={GATE_OUTPIN};
        const uint inputs[]={INTERRUPT_INPIN};
        stdio_init_all(); //initialize all pins
        for(int i=0; i < ELEMENTS(outs); i++) 
        { // Setup each LED in order
            gpio_init(outs[i]);
            gpio_set_dir(outs[i], true);
            gpio_put(outs[i], 1);
        }
        for(int i=0; i < ELEMENTS(inputs); i++) 
        { 
            gpio_init(inputs[i]);
            gpio_set_dir(inputs[i], GPIO_IN);
        }
        /* 
        //The following three lines will be required for analog input delay setting
        adc_init();
        adc_gpio_init(ANALOG);
        adc_select_input(0);
        */
    }

int main()
{    
    float firingDelay_ms = 8.333;
    //uint16_t analogInput;  //variable to store the current analog input value
    stdio_init_all();
    InitInputsandOutputs(); //including the ADC input
    gpio_set_irq_enabled_with_callback(INTERRUPT_INPIN,GPIO_IRQ_EDGE_FALL,true, zeroCrossing_callback); //falling edge
    printf("Welcome to the SCR Timer!(ms)");
    sleep_ms(500);
    while(true)
    {
        /*
        // The next two lines use a potentiometer for setting the delay
        analogInput = adc_read();
        firingDelay_us = map(analogInput, 0, 4095, 0, 8333);  // (IN,INmin,INmax,OUTmin,OUTmax) 0-4095 -> 0-8333;
       */
        printf("Enter the delay in ms: ");
        scanf("%f", &firingDelay_ms);
        firingDelay_us = (uint)(firingDelay_ms * 1000);
        printf("\n");

    }
}
