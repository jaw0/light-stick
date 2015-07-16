/*
  Copyright (c) 2015
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2015-Apr-02 21:37 (EDT)
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

#include "util.h"


#define FLASHSTART	0x08000000
extern const u_char* _etext;
extern int blinky_override;


extern void blinky(void);

// first look on the card, then flash
// NB: if we check the flash first, a bad config could brick the system
#define RUN_SCRIPT(file)        (run_script("fl0:" file))

DEFVAR(int, lights_autostart, 1, UV_TYPE_UL | UV_TYPE_CONFIG, "should we start up the lighting system")


DEFUN(save, "save all config data")
{
    save_config("fl0:config.rc");
    return 0;
}

unsigned int
random(void){
    static unsigned int x;
    unsigned int y;

    y = RTC->SSR ^ RTC->TR;
    y ^= RTC->DR;
    y ^= SysTick->VAL;

    x = (x<<7) | (x>>25);
    x ^= y;
    return x;
}

DEFUN(set_autostart, 0)
{
    if( argc < 2 ) return 0;
    lights_autostart = atoi( argv[1]);
    save_config("fl0:config.rc");
    reboot();
    return 0;
}


#if defined(USE_SSD1306)
#  define dpy_puts	ssd13060_puts
#elif defined(USE_SSD1331)
#  define dpy_puts	ssd13310_puts
#elif defined(USE_EPAPER)
#  define dpy_puts	epaper0_puts
#endif


// on panic: beep, flash, and display message
// NB: system will also output to the console
void
onpanic(const char *msg){
    int i;

    blinky_override = 1;
    set_led_white( 0xFF );
    beep_set(200, 127);

    lights_safe();

    splproc();
    currproc = 0;

    dpy_puts("\e[J\e[16m*** PANIC ***\r\n\e[15m");
    if( msg ){
        dpy_puts(msg);
        dpy_puts("\r\n");
    }
    splhigh();

    while(1){
        set_led_white( 0xFF );
        beep_set(150, 127);
        for(i=0; i<5000000; i++){ asm("nop"); }
        set_led_white( 0x1F );
        beep_set(250, 127);
        for(i=0; i<5000000; i++){ asm("nop"); }
    }
}

//################################################################

// put all peripherals in their lowest power mode
// put the processor into "standby" mode

void
shutdown(void){
    kprintf("powering down\n");
    save_config("fl0:config.rc");

    board_disable();
    power_down();
}


DEFUN(shutdown, "shut down system")
DEFALIAS(shutdown, poweroff)
DEFALIAS(shutdown, halt)
{
    shutdown();
    return 0;
}

//################################################################

void
main(void){

    board_init();
    set_led_white( 0xFF );	// flash LED
    RUN_SCRIPT("config.rc");

    play(6, "b4>f4b4");
    RUN_SCRIPT("startup.rc");

    set_led_white( 0 );
    start_proc(512, blinky, "blinky");

    if( lights_autostart )
        lights_start();

    printf("\n");

    // return to serial shell
}

