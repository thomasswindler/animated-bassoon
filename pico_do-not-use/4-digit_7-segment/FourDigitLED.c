#pragma GCC optimize("O0")

#include <stdio.h>
#include "pico/stdlib.h"

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

#define ON_TIME 2
#define OFF_TIME 0

#define ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))


void setup() {
    const uint outputs[] = {LATCH, CLK, DATA, THOUSANDS, HUNDREDS, TENS, ONES};
    const uint initval[] = {0,     0,   0,    1,         1,        1,    1   };
    stdio_init_all();
    for (int i = 0; i < ELEMENTS(outputs); i++){
        gpio_init(outputs[i]);
        gpio_set_dir(outputs[i],GPIO_OUT);
        gpio_put(outputs[i],initval[i]);
    }
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
int main(void)
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
            }
        }
        else
        {
            count = 0;
        }

        Display(count);
    }
}

