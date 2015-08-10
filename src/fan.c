/*
  Copyright (c) 2015
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2015-Aug-08 16:36 (EDT)
  Function: control fan

*/

#include <conf.h>
#include <proc.h>
#include <i2c.h>
#include <error.h>
#include <stm32.h>
#include <gpio.h>
#include <pwm.h>
#include <userint.h>

#include "board.h"
#include "util.h"
#include "temp.h"

#define FAN_TMIN	(30 * 16)
#define FAN_TMAX	(50 * 16)
#define FAN_VOLT	(3 * 1024)


void
update_fan(void){

    int temp = temp_max();
    int fan  = (temp - FAN_TMIN) * FAN_VOLT;
    fan /= FAN_TMAX - FAN_TMIN;
    fan *= 256;
    fan /= light_vi();

    if( fan < 0 )   fan = 0;
    if( fan > 255 ) fan = 255;

    pwm_set( HWCF_TIMER_FAN, fan);

}

