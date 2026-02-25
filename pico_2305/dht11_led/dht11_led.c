/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "MECHLCDLibrary.h"
#include "MECHws2812.h"

#ifdef PICO_DEFAULT_LED_PIN
#define LED_PIN PICO_DEFAULT_LED_PIN
#endif
#define DHT_PIN 15
#define MAX_TIMINGS 85

typedef struct {
    float humidity;
    float temp_celsius;
} dht_reading;

//web_read_from_dht found here: https://gist.githubusercontent.com/depinette/81d0c6edcdaad67f827896ce1d13b776/raw/09ecc3928ba23fa8f12b00e3d79f160c4e87151d/dht.c
//read_from_dht rewritten from https://github.com/raspberrypi/pico-examples/tree/master/gpio/dht_sensor/
//modified for MECH2305 by D. W. Frame 2022
// added result reporting and code cleanup 
//const uint MAX_TIMINGS = 85;
void web_read_from_dht(dht_reading *result) 
{
    uint data[5] = {0, 0, 0, 0, 0};
    //uint j = 0;
    //___            _____
    //    |___18ms___|
    //controller pulls low for 18-20us
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(20); //18ms
    gpio_put(DHT_PIN, 1);

    //Now controller read the state of the pin
    gpio_set_dir(DHT_PIN, GPIO_IN);

    //___            __________
    //    |___18ms___| 20/40us |
    //wait for 0 (20-40us)
    int countWait = 0;
    for (uint i = 0; i < MAX_TIMINGS; i++) 
    {
        if (gpio_get(DHT_PIN) == 0) {
            break;
        }
        countWait++;
        sleep_us(1);
    }

    //DHT answers by pulling low and high for 80us.
    //___            __________                        ____________________
    //   |___18ms___| 20/40us  |___DHT pulls low 80us__|   then high 80us   |

    //DHT pulls low 80us 
    int count0 = 0;
    for (uint i = 0; i < MAX_TIMINGS; i++) 
    {
        if (gpio_get(DHT_PIN) != 0) 
        {
            break;
        }
        count0++;
        sleep_us(1);
    }

    //DHT pulls high 80us 
    int count1 = 0;
    for (uint i = 0; i < MAX_TIMINGS; i++) 
    {
        if (gpio_get(DHT_PIN) == 0) 
        {
            break;
        }
        count1++;
        sleep_us(1);
    }
    //printf("low=%d, high=%d", count0, count1);

    //DHT then send 40 bits of data
    //for each bit DHT pulls low for 50us 
    //then high for 26-28us for a zero
    //          for    70us for a one
    int nbBits = 0;
    int nbByte = 0;
    while (nbBits < 40) 
    {
        //count 0 duration
        count0 = 0;
        for (uint i = 0; i < MAX_TIMINGS; i++) 
        {
            if (gpio_get(DHT_PIN) != 0) {
                break;
            }
            count0++;
            sleep_us(1);
        }

        //count 1 duration
        count1 = 0;
        for (uint i = 0; i < MAX_TIMINGS; i++) 
        {
            if (gpio_get(DHT_PIN) == 0) 
            {
                break;
            }
            count1++; 
            sleep_us(1);
        }   
        //if level up for more than 26-28us, it is a one
        if (count1 > 30)  
        {
           data[nbByte] = data[nbByte] | (1 << (7 - (nbBits%8))); 
        } 
        nbBits++;

        if ((nbBits%8) == 0)
            nbByte++;
    }
    if ((nbByte >= 5) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) 
    {
        result->humidity = (float) ((data[0] << 8) + data[1]) / 10;
        if (result->humidity > 100) 
        {
            result->humidity = data[0];
        }
        result->temp_celsius = (float) (((data[2] & 0x7F) << 8) + data[3]) / 10;
        if (result->temp_celsius > 125) 
        {
            result->temp_celsius = data[2];
        }
        if (data[2] & 0x80) 
        {
            result->temp_celsius = -result->temp_celsius;
        }
    }
    else 
    {
        printf("Bad data. Raw bytes: [%u, %u, %u, %u, %u]\n", data[0], data[1], data[2], data[3], data[4]); //printf("Bad data\n");
    }
}

// Convert humidity (0-100%) to color gradient: red (0%) to green (100%)
uint32_t humidity_to_color(float humidity) {
    // Linear interpolation from red to green (20-50% range)
    uint8_t green = ((255 * 0.5) * (1.0 - (humidity - 20) / 30.0));//(uint8_t)
    uint8_t red = ((255 * 0.2) * (((humidity - 20) / 30.0)));
    uint8_t blue = 0;//(uint8_t)
    
    return urgb_u32(red, green, blue);
}

// Convert temperature (0-32°C) to color gradient: blue (0°C) to red (32°C)
uint32_t temperature_to_color(float celsius) {
    // Clamp temperature to 0-32°C range
    if (celsius < 0) celsius = 0;
    if (celsius > 32) celsius = 32;
    
    // Linear interpolation from blue to red
    uint8_t red = ((255 * 1) * (celsius / 32.0));
    uint8_t green = 0;
    uint8_t blue = ((255 * 0.2) * (1.0 - celsius / 32.0));
    
    return urgb_u32(red, green, blue);
}

// Animate gradient cycle for a given duration
void animate_gradients(PIO pio, uint sm, uint duration_ms) {
    uint64_t start_time = time_us_64();
    uint64_t duration_us = (uint64_t)duration_ms * 1000;
    
    while ((time_us_64() - start_time) < duration_us) {
        uint64_t elapsed = time_us_64() - start_time;
        float progress = (float)elapsed / duration_us;  // 0.0 to 1.0
        
        // Cycle humidity from 20 to 50
        float humidity_cycle = 20 + (30 * progress);
        
        // Cycle temperature from 0 to 32
        float temp_cycle = 32 * progress;
        
        uint32_t humidity_color = humidity_to_color(humidity_cycle);
        uint32_t temp_color = temperature_to_color(temp_cycle);
        
        put_pixel(pio, sm, humidity_color);
        put_pixel(pio, sm, temp_color);
        
        sleep_ms(50);  // Update every 50ms for smooth animation
    }
}

int main() 
{
    stdio_init_all();
    LCDinit(LCDPINB4, LCDPINB5, LCDPINB6, LCDPINB7, LCDPINRS, LCDPINE, COLUMNS, ROWS);
    gpio_init(DHT_PIN);

    // Initialize WS2812 LEDs
    PIO pio;
    uint sm;
    uint offset;
    
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // DHT sensor warm-up delay
    //sleep_ms(2000);

    #ifdef LED_PIN
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
    #endif
    
    int update_count = 0;  // Counter for LCD updates
    
    while (1) 
    {
        dht_reading reading;
        web_read_from_dht(&reading);

        float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
        printf("Humidity = %.1f%%, Temp = %.1fC (%.1fF)\n",
               reading.humidity, reading.temp_celsius, fahrenheit);
        
        // Display data on LCD

        char humidity_buffer[17];

        char temp_buffer[17];

        sprintf(humidity_buffer, "Humidity = %.1f%%", reading.humidity);

        sprintf(temp_buffer, "T=%.1fC(%.1fF)", reading.temp_celsius, fahrenheit);

        LCDclear();

        LCDprint(humidity_buffer);

        LCDgotoPos(0, 1);

        LCDprint(temp_buffer);

        // Update WS2812 LEDs
        uint32_t humidity_color = humidity_to_color(reading.humidity);
        uint32_t temp_color = temperature_to_color(reading.temp_celsius);
        
        put_pixel(pio, sm, humidity_color);  // First LED: humidity gradient (red to green)
        put_pixel(pio, sm, temp_color);      // Second LED: temperature gradient (blue to red)

        sleep_ms(2000);
        
        // Every 4 updates, run 2-second animation
        update_count++;
        if (update_count >= 4) {
            animate_gradients(pio, sm, 1500);
            update_count = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------//
// LCD library code for 16x2 LCD connected to Raspberry Pi Pico
// David Frame Mechatronics Utah Valley University 2022
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include "MECHLCDLibrary.h"

struct LCDInstance
{
    int LCDpins[6];
    uint32_t LCDmask_c; // with clock
    uint32_t LCDmask;   // without clock
    int no_chars;
    int no_lines;
    int cursor_status[2];
} myLCD;

uint32_t pinValuesToMask(uint raw_bits[], int length)
{ // Array of Bit 7, Bit 6, Bit 5, Bit 4, RS(, clock)
    uint32_t result = 0;
    uint pinArray[32];
    for (int i = 0; i < 32; i++)
    {
        pinArray[i] = 0;
    }
    for (int i = 0; i < length; i++)
    {
        pinArray[myLCD.LCDpins[i]] = raw_bits[i];
    }
    for (int i = 0; i < 32; i++)
    {
        result = result << 1;
        result += pinArray[31 - i];
    }
    return result;
}

void uintInto8bits(uint raw_bits[], uint one_byte)
{
    for (int i = 0; i < 8; i++)
    {
        raw_bits[7 - i] = one_byte % 2;
        one_byte = one_byte >> 1;
    }
}

void sendRawDataOneCycle(uint raw_bits[])
{ // Array of Bit 7, Bit 6, Bit 5, Bit 4, RS
    uint32_t bit_value = pinValuesToMask(raw_bits, 5);
    gpio_put_masked(myLCD.LCDmask, bit_value);
    gpio_put(myLCD.LCDpins[E], HIGH);
    sleep_us(PIN_E_SLEEP_US);
    gpio_put(myLCD.LCDpins[E], LOW); // gpio values on other pins are pushed at the HIGH->LOW change of the clock.
    sleep_us(PIN_E_SLEEP_US);
}

void sendFullByte(uint rs, uint databits[])
{ // RS + array of Bit7, ... , Bit0
    // send upper nibble (MSN)
    uint rawbits[5];
    rawbits[4] = rs;
    for (int i = 0; i < 4; i++)
    {
        rawbits[i] = databits[i];
    }
    sendRawDataOneCycle(rawbits);
    // send lower nibble (LSN)
    for (int i = 0; i < 4; i++)
    {
        rawbits[i] = databits[i + 4];
    }
    sendRawDataOneCycle(rawbits);
}

void LCDinit(int bit4_pin, int bit5_pin, int bit6_pin, int bit7_pin, int rs_pin, int e_pin, int width, int depth)
{

    uint all_ones[6] = {1, 1, 1, 1, 1, 1};
    uint set_function_8[5] = {0, 0, 1, 1, 0};
    uint set_function_4a[5] = {0, 0, 1, 0, 0};

    uint set_function_4[8] = {0, 0, 1, 0, 0, 0, 0, 0};
    uint cursor_set[8] = {0, 0, 0, 0, 0, 1, 1, 0};
    uint display_prop_set[8] = {0, 0, 0, 0, 1, 1, 0, 0};

    myLCD.LCDpins[0] = bit7_pin;
    myLCD.LCDpins[1] = bit6_pin;
    myLCD.LCDpins[2] = bit5_pin;
    myLCD.LCDpins[3] = bit4_pin;
    myLCD.LCDpins[4] = rs_pin;
    myLCD.LCDpins[5] = e_pin;
    myLCD.no_chars = width;
    myLCD.no_lines = depth;

    // set mask, initialize masked pins and set to LOW
    myLCD.LCDmask_c = pinValuesToMask(all_ones, 6);
    myLCD.LCDmask = pinValuesToMask(all_ones, 5);
    gpio_init_mask(myLCD.LCDmask_c);          // init all LCDpins
    gpio_set_dir_out_masked(myLCD.LCDmask_c); // Set as output all LCDpins
    gpio_clr_mask(myLCD.LCDmask_c);           // LOW on all LCD pins

    // set LCD to 4-bit mode and 1 or 2 lines
    // by sending a series of Set Function commands to secure the state and set to 4 bits
    if (myLCD.no_lines == 2)
    {
        set_function_4[4] = 1;
    };
    sendRawDataOneCycle(set_function_8);
    sendRawDataOneCycle(set_function_8);
    sendRawDataOneCycle(set_function_8);
    sendRawDataOneCycle(set_function_4a);

    // getting ready
    sendFullByte(COMMAND, set_function_4);
    sendFullByte(COMMAND, cursor_set);
    sendFullByte(COMMAND, display_prop_set);
    LCDclear();
    myLCD.cursor_status[0] = 0;
    myLCD.cursor_status[1] = 0;

}

void LCDclear()
{
    uint clear_display[8] = {0, 0, 0, 0, 0, 0, 0, 1};
    sendFullByte(COMMAND, clear_display);
    sleep_us(LCD_CLEAR_DELAY_US); // extra sleep due to equipment time needed to clear the display
}

void cursor_off()
{
    uint no_cursor[8] = {0, 0, 0, 0, 1, 1, 0, 0};
    sendFullByte(COMMAND, no_cursor);
    myLCD.cursor_status[0] = 0;
    myLCD.cursor_status[1] = 0;
}


void LCDcursorOn(bool blink)
{
    uint command_cursor[8] = {0, 0, 0, 0, 1, 1, 1, 0};
    if (blink)
        command_cursor[7] = 1;
    sendFullByte(COMMAND, command_cursor);
    myLCD.cursor_status[0] = 1;
    myLCD.cursor_status[1] = command_cursor[7];
}

void LCDdisplayOff()
{
    uint command_display[8] = {0, 0, 0, 0, 1, 0, 0, 0};
    command_display[7] = myLCD.cursor_status[1];
    command_display[6] = myLCD.cursor_status[0];
    sendFullByte(COMMAND, command_display);
}

void LCDdisplayOn()
{
    uint command_display[8] = {0, 0, 0, 0, 1, 1, 0, 0};
    command_display[7] = myLCD.cursor_status[1];
    command_display[6] = myLCD.cursor_status[0];
    sendFullByte(COMMAND, command_display);
}

void LCDgotoPos(int pos_i, int line)
{
    uint eight_bits[8];
    uint pos = (uint)pos_i;
    switch (myLCD.no_lines)
    {
    case 2:
        pos = 64 * line + pos + 0b10000000;
        break;
    case 4:
        if (line == 0 || line == 2)
        {
            pos = 64 * (line / 2) + pos + 0b10000000;
        }
        else
        {
            pos = 64 * ((line - 1) / 2) + 20 + pos + 0b10000000;
        };
        break;
    default:
        pos = pos;
    };
    uintInto8bits(eight_bits, pos);
    sendFullByte(COMMAND, eight_bits);
}

void LCDprint(const char *str)
{
    uint eight_bits[8];
    int i = 0;
    while (str[i] != 0)
    {
        uintInto8bits(eight_bits, (uint)(str[i]));
        sendFullByte(DATA, eight_bits);
        ++i;
    }
}

void LCDprintWrapped(const char *str)
{
    uint eight_bits[8];
    int line = 0;
    int charInLine = 0;
    int i = 0;

    LCDgotoPos(0, 0);

    while (str[i] != 0)
    {
        if (str[i] == 10) // if there is a new line char in the string (\n) start the next line
        {
            // this will make the program act as though the line has run out of space
            charInLine = myLCD.no_chars;
        }
        else
        {
            // if there is not a newline, print the next char
            uintInto8bits(eight_bits, (uint)(str[i]));
            sendFullByte(DATA, eight_bits);

            // update so we know the location in the line
            ++charInLine;
        }

        // if there is no room in this line, start the next one.
        if (charInLine == myLCD.no_chars)
        {
            ++ line;
            LCDgotoPos(0, line);
        }
        ++i;
    }
}

void LCDWriteStringXY(int x,int y, const char *msg) 
{
 LCDgotoPos(x,y);
 LCDprint(msg);
}

void LCDWriteIntXY(int x, int y, int var)
{
    char buffer[6]={0,0,0,0,0,0};
    sprintf(buffer,"%4.4d", var);
    LCDWriteStringXY(x,y,buffer);
}

void LCDWriteFloatXY(int x, int y, float var)
{
    char buffer[6]={0,0,0,0,0,0};
    sprintf(buffer,"%4.1f", var);
    LCDWriteStringXY(x,y,buffer);
}

// the following is a test program to test the library.
// to run this program, you need to include the library in the same folder as this file.
int main_MECHLCDLibrary()
{
    int variable = 0;
    char buffer[33];
    stdio_init_all();
    LCDinit(LCDPINB4, LCDPINB5, LCDPINB6, LCDPINB7, LCDPINRS, LCDPINE, COLUMNS, ROWS);
    
    while(true)
    {
        LCDprint(" MECH Menu");
        LCDgotoPos(0,1);
        LCDprint("var1:");
        LCDcursorOn(true);
        LCDWriteIntXY(5,1,variable++);
        LCDWriteFloatXY(10,1,(float)variable);
        sleep_ms(5000);
        LCDclear();
        sleep_ms(1000);
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
