/*
  Copyright (c) 2015
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2015-Jun-15 20:25 (EDT)
  Function: read temperature sensors (TMP100)

*/

#include <conf.h>
#include <proc.h>
#include <i2c.h>
#include <error.h>
#include <stm32.h>
#include <userint.h>

#include "board.h"
#include "util.h"
#include "temp.h"

short tempbuf[3];

static i2c_msg tempinit[] = {
    I2C_MSG_C2( TMP100_ADDRESS_0,   0, TMP100_REGISTER_CONFIG, 0 ),	// enabled, 9 bits
    I2C_MSG_C2( TMP100_ADDRESS_1,   0, TMP100_REGISTER_CONFIG, 0 ),
    I2C_MSG_C2( TMP100_ADDRESS_2,   0, TMP100_REGISTER_CONFIG, 0 ),

};

static i2c_msg tempoff[] = {
    I2C_MSG_C2( TMP100_ADDRESS_0,   0, TMP100_REGISTER_CONFIG, 1 ),	// shutdown
    I2C_MSG_C2( TMP100_ADDRESS_1,   0, TMP100_REGISTER_CONFIG, 1 ),
    I2C_MSG_C2( TMP100_ADDRESS_2,   0, TMP100_REGISTER_CONFIG, 1 ),
};

static i2c_msg tempreadall[] = {
    I2C_MSG_C1( TMP100_ADDRESS_2,   0,             TMP100_REGISTER_TEMP ),
    I2C_MSG_DL( TMP100_ADDRESS_2,   I2C_MSGF_READ, 2, tempbuf + 2 ),

    I2C_MSG_C1( TMP100_ADDRESS_0,   0,             TMP100_REGISTER_TEMP ),
    I2C_MSG_DL( TMP100_ADDRESS_0,   I2C_MSGF_READ, 2, tempbuf + 0 ),

    I2C_MSG_C1( TMP100_ADDRESS_1,   0,             TMP100_REGISTER_TEMP ),
    I2C_MSG_DL( TMP100_ADDRESS_1,   I2C_MSGF_READ, 2, tempbuf + 1 ),

};



void
temp_init(void){
    char i;

    // init
    i2c_xfer(I2CUNIT, ELEMENTSIN(tempinit), tempinit, 1000000);
    bootmsg("tmp100 x 3 on i2c%d\n", I2CUNIT);
}

void
temp_disable(void){
    i2c_xfer(I2CUNIT, ELEMENTSIN(tempoff), tempoff, 1000000);
}

void
read_temp_all(void){
    i2c_xfer(I2CUNIT, ELEMENTSIN(tempreadall), tempreadall, 100000);

    // byte swap + shift 4
    tempbuf[0] = ((tempbuf[0]&0xFF) << 4) | ((tempbuf[0]&0xF00) >> 8);
    tempbuf[1] = ((tempbuf[1]&0xFF) << 4) | ((tempbuf[1]&0xF00) >> 8);
    tempbuf[2] = ((tempbuf[2]&0xFF) << 4) | ((tempbuf[2]&0xF00) >> 8);
}

/****************************************************************/

int
temp_ctl(void){
    return tempbuf[2];
}
int
temp_mid(void){
    return tempbuf[0];
}

int
temp_far(void){
    return tempbuf[1];
}

int
temp_max(void){
    int temp = tempbuf[0];
    if( tempbuf[1] > temp ) temp = tempbuf[1];
    if( tempbuf[2] > temp ) temp = tempbuf[2];

    return temp;
}

/****************************************************************/

DEFUN(testtemp, "test temp")
{
    tempbuf[0] = tempbuf[1] = tempbuf[2] = 0xDEAD;
    while(1){
        read_temp_all();

        if( argv[0][0] == '-' ) printf("\e[J");
        printf("%3d %3d %3d\n", tempbuf[0], tempbuf[1], tempbuf[2]);
        printf("\n");

        if( argv[0][0] != '-' ) break;
        if( check_button() ) break;
        if( check_menu_button() ) break;

        usleep(250000);
    }
    return 0;
}

