/*
  Copyright (c) 2015
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2015-Jul-09 20:48 (EDT)
  Function: 

*/

#include <conf.h>
#include <proc.h>
#include <msgs.h>
#include <gpio.h>
#include <pwm.h>
#include <ioctl.h>
#include <stm32.h>
#include <userint.h>

#include "board.h"
#include "util.h"

extern int ivolume;
extern int keypress;

static inline void
effect_a(void){
    //set_leds_rgb( 0x800080, 0x800080 );
    play(ivolume, "A4>>");
}
static inline void
effect_b(void){
    //set_leds_rgb( 0x800080, 0x800080 );
    play(ivolume, "D-4>>");
    //set_leds_rgb( 0, 0 );
}

int
check_button(void){

    if( gpio_get( HWCF_GPIO_BUTTON ) ){
        effect_a();

        while( gpio_get( HWCF_GPIO_BUTTON ) ){
            // input_read_all();
            usleep(10000);
        }
        effect_b();
        return 1;
    }
    return 0;
}

int
check_menu_button(void){

    int c = keypress;
    keypress = 0;

    if( c == 'm' ){
        effect_a();
        return 1;
    }

    if( c ){
        effect_b();
    }

    return 0;
}

