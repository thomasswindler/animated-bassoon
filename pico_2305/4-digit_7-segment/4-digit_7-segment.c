#pragma GCC optimize("O0")

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

/*
 * FourDigitLEDTest.c
 * Adapted from Arudio C code
 * Created: 4/20/2020 5:07:40 PM
 * Author : dframe (david.frame@uvu.edu)
 */ 

// Here are the hardware connections to the Pico:
#define LATCH 5     //74HC595  pin 12 RCLK
#define CLK 6       //74HC595  pin 11 SRCLK
#define DATA 7      //74HC595  pin 14 SER
#define THOUSANDS 0	//resistor->Four digit display 12 Low to enable
#define HUNDREDS 1	//resistor->Four digit display 9 Low to enable
#define TENS 2		//resistor->Four digit display 8 Low to enable
#define ONES 3		//resistor->Four digit display 6 Low to enable

#define TOGGLE_PIN 16 // GPIO used to toggle pull state inside main loop

#define ON_TIME 2
#define OFF_TIME 0

#define ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

volatile uint count = 9999;

void setup() {
    const uint outputs[] = {LATCH, CLK, DATA, THOUSANDS, HUNDREDS, TENS, ONES};
    const uint initval[] = {0,     0,   0,    1,         1,        1,    1   };
    stdio_init_all();
    for (int i = 0; i < ELEMENTS(outputs); i++){
        gpio_init(outputs[i]);
        gpio_set_dir(outputs[i],GPIO_OUT);
        gpio_put(outputs[i],initval[i]);
    }

    // Initialize the toggle pin as an output (start low)
    gpio_init(TOGGLE_PIN);
    gpio_set_dir(TOGGLE_PIN, GPIO_OUT);
    gpio_put(TOGGLE_PIN, false);
}

void static inline shiftOut(char num){
	gpio_put(LATCH, false); //latch low
	for(char i = 0; i < 8;i++){
		gpio_put(CLK, false);      //CLK low
		if ((num & 0x80) == 0x80)
        { // is the MSB a one?
			gpio_put(DATA, true);  // data pin HIGH
		}
		else
        {
			gpio_put(DATA, false); // data pin LOW
		}
        asm("NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; \
            NOP; NOP; NOP; NOP; NOP; NOP;"); //trying to meet 100ns setup time
		gpio_put(CLK,true);       //CLK pin high
        asm( "NOP; NOP; NOP; NOP; NOP; NOP; NOP; \
            NOP;"); //trying to meet 100ns high time minimum for the clock
		num = num << 1;            // left shift to move the next bit to MSB
	}
	asm( "NOP; NOP; NOP;");
    gpio_put(LATCH, true); //high
	sleep_ms(1);
}

// Original Display function (commented out)
/*
void static inline Display(unsigned int num)
{
    uint8_t digitEnable[] = {THOUSANDS,HUNDREDS,TENS,ONES};
    uint16_t digitSelect[]= {1000, 100, 10, 1};
    unsigned char table[]=
    {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,
        0x77,0x7c,0x39,0x5e,0x79,0x71,0x00};	
	int digit;
	for(uint8_t i = 0; i < 4; i++){		 
		digit = num / digitSelect[i];		//separate and select a digit
		num -= digit * digitSelect[i];
										// display the digit
		gpio_put(digitEnable[i], false);	//enable digit
        		shiftOut(table[digit]);
		sleep_ms(ON_TIME);
		gpio_put(digitEnable[i], 1);		//disable digit
	}
	sleep_ms(OFF_TIME);
}
*/

// Modified Display function with decimal point support
// decimalPosition: 0-3 for which digit shows decimal (0=thousands, 1=hundreds, 2=tens, 3=ones)
// Use 255 or 4+ for no decimal point
void static inline Display(unsigned int num, uint8_t decimalPosition)
{
    uint8_t digitEnable[] = {THOUSANDS,HUNDREDS,TENS,ONES};
    uint16_t digitSelect[]= {1000, 100, 10, 1};
    unsigned char table[]=
    {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,
        0x77,0x7c,0x39,0x5e,0x79,0x71,0x00};	
	int digit;
	for(uint8_t i = 0; i < 4; i++){		 
		digit = num / digitSelect[i];		//separate and select a digit
		num -= digit * digitSelect[i];
		
		// Get the 7-segment pattern for this digit
		unsigned char pattern = table[digit];
		
		// Set decimal point bit (0x80) if this is the decimal position
		if (i == decimalPosition) {
			pattern |= 0x80;  // Set bit 7 for decimal point
		}
		
		// display the digit
		gpio_put(digitEnable[i], false);	//enable digit
        		shiftOut(pattern);
		sleep_ms(ON_TIME);
		gpio_put(digitEnable[i], 1);		//disable digit
	}
	sleep_ms(OFF_TIME);
}


bool repeating_timer_callback(struct repeating_timer *t)
{
    // toggle a probe pin so the scope can see the timer firing
    gpio_put(TOGGLE_PIN, !gpio_get(TOGGLE_PIN));

    // only update the shared variable; do not touch the display or sleep here
    if (count > 0u) {
        count--;
    } else {
        count = 9999;
    }

    return true; // keep the timer repeating
}

int count_up_main(void)
{
    uint count = 2715;
    uint32_t t1,t2;
    setup();
    t1 = time_us_32();
    while (1) 
    {  
		if (count < 9999)
        {
            t2 = time_us_32();
            if ((t2-t1) > 999999)
            {
                count++;
                t1 = t2;
                gpio_put(TOGGLE_PIN, !gpio_get(TOGGLE_PIN));
            }
        }
    
        else
        {
            count = 0;
        }
        
        Display(count, 255);  // Display with decimal point at tens place i.e. 27.15
    }
}


int count_down_main(void) //Count-down code for the other portion of the lab.
{
    uint count = 9999;
    uint32_t t1,t2;
    setup();
    t1 = time_us_32();
    while (1) 
    {
        gpio_put(TOGGLE_PIN, !gpio_get(TOGGLE_PIN));
		if (count > 0000)
        {
            t2 = time_us_32();
            if ((t2-t1) > 9999) //
            {
                count--; //decrement count
                t1 = t2;
            }
        }
        else
        {
            count = 9999;
        }

        Display(count, 1);  // Display with decimal point at tens place i.e. 99.99

        // set the toggle pin LOW at the end of the loop
    }
}


int main(void) // Count‑down using hardware timer for the counter
{
    setup();
    stdio_init_all();

    struct repeating_timer timer;

    add_repeating_timer_ms(1, repeating_timer_callback, NULL, &timer); //decrease the count every 1ms

    while (1)  //Updates the display in the main loop, but the count is updated in the timer callback
    {
        Display(count, 1);   // decimal point on hundreds place (99.99)
        
        // decreases the cpu usage since the display doesn't need to be updated as precisely
        // This could be removed since the display function has a delay in it
        // Having too large of a time would cause the display to flicker, I picked a low value for no particular reason.
        // This could easily be set to 1 ms and it would work fine.
        sleep_us(100); 

    }
}
