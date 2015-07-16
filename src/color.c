/*
  Copyright (c) 2015
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2015-Jun-16 00:02 (EDT)
  Function: color sensor BH1745NUC

*/

#include <conf.h>
#include <proc.h>
#include <i2c.h>
#include <error.h>
#include <stm32.h>
#include <userint.h>

#include "board.h"
#include "util.h"
#include "color.h"

short colorbuf[4];

static i2c_msg colorinit[] = {
    I2C_MSG_C2( BH1745_ADDRESS,   0, BH1745_REGISTER_MODE_CONTROL1, 0 ),	// 160ms
    I2C_MSG_C2( BH1745_ADDRESS,   0, BH1745_REGISTER_MODE_CONTROL2, 0x90 ),	// active, gain=1
};

static i2c_msg coloroff[] = {
    I2C_MSG_C2( BH1745_ADDRESS,   0, BH1745_REGISTER_MODE_CONTROL2, 0 ),	// shutdown
};

static i2c_msg colorreadall[] = {
    I2C_MSG_C1( BH1745_ADDRESS,   0,             BH1745_REGISTER_RED_LSB ),
    I2C_MSG_DL( BH1745_ADDRESS,   I2C_MSGF_READ, 8, colorbuf ),

};

/****************************************************************/

void
color_init(void){
    char i;

    // init
    i2c_xfer(I2CUNIT, ELEMENTSIN(colorinit), colorinit, 1000000);
    bootmsg("bh1745 on i2c%d\n", I2CUNIT);
}

void
color_disable(void){
    i2c_xfer(I2CUNIT, ELEMENTSIN(coloroff), coloroff, 1000000);
}

void
read_color_all(void){
    i2c_xfer(I2CUNIT, ELEMENTSIN(colorreadall), colorreadall, 100000);
}

/****************************************************************/
int color_r(void){ return colorbuf[0]; }
int color_g(void){ return colorbuf[1]; }
int color_b(void){ return colorbuf[2]; }

int
color_rgb(void){
    return ((colorbuf[0] & 0xFF00) <<8) | (colorbuf[1] & 0xFF00) | (colorbuf[2]>>8);
}

/****************************************************************/
// http://dsp.stackexchange.com/questions/8949/how-do-i-calculate-the-color-temperature-of-the-light-source-illuminating-an-ima


DEFUN(testcolor, "test color sensor")
{

    while(1){
        read_color_all();
        if( argv[0][0] == '-' ) printf("\e[J");
        printf("  R    G    B    C  \n");
        printf("%04.4x %04.4x %04.4x %04.4x\n", colorbuf[0], colorbuf[1], colorbuf[2], colorbuf[3]);
        printf("\n");

        if( argv[0][0] != '-' ) break;
        if( check_button() ) break;
        if( check_menu_button() ) break;

        usleep(250000);
    }
    return 0;
}

