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


int main() 
{
    stdio_init_all();
 
    gpio_init(DHT_PIN);

    // DHT sensor warm-up delay
    //sleep_ms(2000);

    #ifdef LED_PIN
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
    #endif
    while (1) 
    {
        dht_reading reading;
        web_read_from_dht(&reading);

        float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
        printf("Humidity = %.1f%%, Temp = %.1fC (%.1fF)\n",
               reading.humidity, reading.temp_celsius, fahrenheit);
        
        sleep_ms(2000);
    }
}

