/*
  Copyright (c) 2015
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2015-Apr-02 21:36 (EDT)
  Function: 

*/

#include <conf.h>
#include <proc.h>
#include <gpio.h>
#include <pwm.h>
#include <adc.h>
#include <spi.h>
#include <i2c.h>
#include <ioctl.h>
#include <error.h>
#include <stm32.h>
#include <userint.h>

#include "board.h"


// hwinit      - called early in init process, to init hw
// board_init  - always runs

void
hwinit(void){

    // enable power
    gpio_init( HWCF_GPIO_POWEREN, GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_SPEED_25MHZ );
    gpio_set(  HWCF_GPIO_POWEREN );

}

void
board_init(void){
    bootmsg("board hw init\n");

    // enable i+d cache, prefetch=off => faster + lower adc noise
    // nb: prefetch=on => more faster, less power, more noise
    FLASH->ACR  |= 0x600;

    // Timer 1+2 => AF(1), Timer 3+4+5 => AF(2)
    // beeper
    gpio_init( HWCF_GPIO_AUDIO, GPIO_AF(2) | GPIO_SPEED_25MHZ );
    pwm_init(  HWCF_TIMER_AUDIO, 440, 255 );
    pwm_set(   HWCF_TIMER_AUDIO, 0);

    // small LED
    gpio_init( HWCF_GPIO_LED_WHITE,   GPIO_AF(2) | GPIO_SPEED_25MHZ );
    pwm_set(   HWCF_TIMER_LED_WHITE,  0);

    // button
    gpio_init( HWCF_GPIO_BUTTON,      GPIO_INPUT );

    // high power drivers
    gpio_init( HWCF_GPIO_LEDS0,   GPIO_AF(1) | GPIO_SPEED_25MHZ );
    pwm_init(  HWCF_TIMER_LEDS0,  93000, 1023);
    pwm_set(   HWCF_TIMER_LEDS0,  0);

    gpio_init( HWCF_GPIO_LEDS1,   GPIO_AF(2) | GPIO_SPEED_25MHZ );
    pwm_init(  HWCF_TIMER_LEDS1,  93000, 1023);
    pwm_set(   HWCF_TIMER_LEDS1,  0);

    // fan
    gpio_init( HWCF_GPIO_FAN,     GPIO_AF(1) | GPIO_SPEED_25MHZ );
    pwm_init(  HWCF_TIMER_FAN,    10000, 255);
    pwm_set(   HWCF_TIMER_FAN,    0);

    // ADCs
    adc_init( HWCF_ADC_BATV, 1 );
    adc_init( HWCF_ADC_L0I,  1 );
    adc_init( HWCF_ADC_L0V,  1 );
    adc_init( HWCF_ADC_L1I,  1 );
    adc_init( HWCF_ADC_L1V,  1 );


    lights_init();
    light_adc_init();
    temp_init();
    touch_init();
    color_init();

    // init temp, color sensors, touch pad
    // ...
}

void
board_disable(void){

    // lights out
    pwm_set(   HWCF_TIMER_LEDS0,  0);
    pwm_set(   HWCF_TIMER_LEDS1,  0);

    // turn off power
    gpio_clear(  HWCF_GPIO_POWEREN );

    // i2c devices will be powered off, no need to explicitly disable them
}


void
debug_set_led(int v){
    static int initp = 0;

    if( !initp ){
        gpio_init( HWCF_GPIO_LED_WHITE,  GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_SPEED_25MHZ );
        initp = 1;
    }

    if( v )
        gpio_set( HWCF_GPIO_LED_WHITE );
    else
        gpio_clear( HWCF_GPIO_LED_WHITE );

}

