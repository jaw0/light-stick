/*
  Copyright (c) 2015
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2015-Jul-11 15:55 (EDT)
  Function: cap1298 touch sensor

*/



#include <conf.h>
#include <proc.h>
#include <i2c.h>
#include <error.h>
#include <stm32.h>
#include <userint.h>

#include "board.h"
#include "util.h"
#include "touch.h"


// menu, down, right, ok, left, up, b, a

u_char capbuf[4];

static i2c_msg touchinit[] = {
    I2C_MSG_C2( CAP1298_ADDRESS,   0, CAP1298_REGISTER_MAIN_CONTROL,  0 ),	// active
    I2C_MSG_C2( CAP1298_ADDRESS,   0, CAP1298_REGISTER_INPUT_ENABLE,  0xFF ),	// all enabled
    I2C_MSG_C2( CAP1298_ADDRESS,   0, CAP1298_REGISTER_INPUT_CONFIG,  0xE6 ),	// recal 10sec, repeat rate 245ms
    I2C_MSG_C2( CAP1298_ADDRESS,   0, CAP1298_REGISTER_INPUT_CONFIG2, 0x0D ),	// press 490ms
    I2C_MSG_C2( CAP1298_ADDRESS,   0, CAP1298_REGISTER_REPEAT_ENABLE, 0x36 ),	// only arrow keys repeat
    I2C_MSG_C2( CAP1298_ADDRESS,   0, CAP1298_REGISTER_CONFIG2,       0x41 ),	// no release int
};

static i2c_msg touchoff[] = {
    I2C_MSG_C2( CAP1298_ADDRESS,   0, CAP1298_REGISTER_MAIN_CONTROL, 0x10 ),	// deep sleep
};

static i2c_msg touchprobe[] = {
    I2C_MSG_C1( CAP1298_ADDRESS,  0,             CAP1298_REGISTER_PRODUCT_ID ),
    I2C_MSG_DL( CAP1298_ADDRESS,  I2C_MSGF_READ, 3, capbuf + 0 ),
};

static i2c_msg touchreadall[] = {
    // main control, (undef), general status + input status in one read
    I2C_MSG_C1( CAP1298_ADDRESS,  0,             CAP1298_REGISTER_MAIN_CONTROL ),
    I2C_MSG_DL( CAP1298_ADDRESS,  I2C_MSGF_READ, 4, capbuf + 0 ),
};

static i2c_msg touchintack[] = {
    // ack any pending int
    I2C_MSG_C2( CAP1298_ADDRESS,  0, CAP1298_REGISTER_MAIN_CONTROL, 0 ),
};



void
touch_init(void){
    char i;

    // init
    i2c_xfer(I2CUNIT, ELEMENTSIN(touchinit), touchinit, 1000000);
    // try to read
    i2c_xfer(I2CUNIT, ELEMENTSIN(touchprobe), touchprobe, 1000000);

    if( capbuf[0] == CAP1298_PRODUCT_ID ){
        bootmsg("cap1298 on i2c%d rev. %d\n", I2CUNIT, capbuf[2]);
    }
}

void
touch_disable(void){
    i2c_xfer(I2CUNIT, ELEMENTSIN(touchoff), touchoff, 1000000);
}

int
read_touch_all(void){
    i2c_xfer(I2CUNIT, ELEMENTSIN(touchreadall), touchreadall, 100000);
    // ack int
    i2c_xfer(I2CUNIT, ELEMENTSIN(touchintack),  touchintack,  100000);

    // int bit set?
    if( capbuf[0] & 1 ){

        // multi touch?
        if( capbuf[2] & 4 ) return '\a';

        int c = capbuf[3];
        // menu, down, right, ok, left, up, b, a
        if( c & (1 << 0) ) return 'm';
        if( c & (1 << 1) ) return 'v';
        if( c & (1 << 2) ) return '>';
        if( c & (1 << 3) ) return '\n';
        if( c & (1 << 4) ) return '<';
        if( c & (1 << 5) ) return '^';
        if( c & (1 << 6) ) return 'b';
        if( c & (1 << 7) ) return 'a';
    }

    return 0;
}

DEFUN(testtouch, "test touch sensor")
{

    i2c_xfer(I2CUNIT, ELEMENTSIN(touchprobe), touchprobe, 1000000);
    // printf("> %02.2x %02.2x %02.2x\n", capbuf[0], capbuf[1], capbuf[2]);

    while(1){
        int c = read_touch_all();
        if(c) printf(">> [%02.2x] %02.2x %02.2x %02.2x\n", c, capbuf[0], capbuf[2], capbuf[3]);
        if( check_button() ) break;
        usleep(1000);
    }
    return 0;
}

