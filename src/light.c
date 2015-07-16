/*
  Copyright (c) 2015
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2015-Jul-09 20:44 (EDT)
  Function: 

*/


#include <conf.h>
#include <proc.h>
#include <stm32.h>
#include <nvic.h>
#include <gpio.h>
#include <adc.h>
#include <pwm.h>
#include <ioctl.h>
#include <userint.h>

#include "board.h"
#include "util.h"
#include "uimenu.h"

// safety cutoff levels:
#define MAX_VOUT	(40*1024)
#define MAX_IOUT	(10*1024)

#define FAN_TEMP	(40*16)		// start fan at this temp, celsius
#define MAX_TEMP	(50*16)		// max temp, celsius
#define MAX_PWM		1000		// ~95%
#define MAX_PSET	40000		// max power setting
#define PWR_STEP	  500		// power level step size

// QQQ - power level units? mA, watts, lumens, percent, fraction, f-stop, ...?
DEFVAR(int,  power_level,   PWR_STEP,  UV_TYPE_UL | UV_TYPE_CONFIG, "power level setting")
DEFVAR(int,  white_balance, 4600,      UV_TYPE_UL | UV_TYPE_CONFIG, "white balance setting")

extern int ivolume;
extern int blinky_override;
int keypress = 0;
static bool disco_mode = 0;
static bool safety_override = 0;
static int i0_off = 0, i1_off = 0;
static int i0f = 0, i1f = 0;
static int pwm0 = 0, pwm1 = 0;

void
lights_init(void){

}

static void
lights_offset(void){
    // find offset
    // QQQ - where does this come from?
    char i;

    i0_off = i1_off = 0;

    for(i=0; i<100; i++){
        light_adc_read();
        i0_off += light_i0();
        i1_off += light_i1();

        usleep(1000);
    }

    i0_off /= 100;
    i1_off /= 100;

    //bootmsg("light Ioff %d %d\n", i0_off, i1_off);
}

void
check_power_switch(void){

    if( gpio_get(HWCF_GPIO_PSWITCH) ){
        play(ivolume, "b4a4>>d-4");
        // recheck
        if( gpio_get(HWCF_GPIO_PSWITCH) )
            shutdown();
    }
}

void
lights_safe(void){
    pwm_set(   HWCF_TIMER_LEDS0,  0);
    pwm_set(   HWCF_TIMER_LEDS1,  0);
}

static void
safety_check(void){
    const char *err = 0;

    if( temp_ctl() > MAX_TEMP || temp_mid() > MAX_TEMP || temp_far() > MAX_TEMP ){
        err = "overheat";
    }
    if( light_v0() > MAX_VOUT || light_v1() > MAX_VOUT || light_i0() > MAX_IOUT || light_i1() > MAX_IOUT ){
        err = "overload";
    }
    if( !err ) return;

    lights_safe();

    // until power off or reset
    while(1){
        blinky_override = 1;
        safety_override = 1;
        light_adc_read();
        read_temp_all();
        lights_safe();
        check_power_switch();

        set_led_white( 0xFF );
        beep_set(150, 127);
        printf("\e[J");
        set_font("10x20");
        printf("* %s *\n", err);
        set_font("5x8");
        printf("v: %6d %6d\ni: %6d %6d\n", light_v0(), light_v1(), light_i0(), light_i1() );
        printf("t: %4d %4d %4d\n", temp_ctl(), temp_mid(), temp_far());
        usleep( 500000 );
        set_led_white( 0x1F );
        beep_set(250, 127);
        usleep( 500000 );
    }
}

/****************************************************************/

// incand/bulb: 2800, tungsten 3200, flour 4600, day 5600, sky 6000

DEFUN(set_power_level, 0)
{
    int c = menu_get_int("power level", "milliwatts", 0, MAX_PSET, PWR_STEP, &power_level);
    if( c < 0 ) return c;
    power_level = c;
    return MFRET_BACK;
}
DEFUN(set_white_balance, 0)
{
    int c = menu_get_int("white bal", "degrees kelvin", 2700, 6500, 100, &white_balance);
    if( c < 0 ) return c;
    white_balance = c;
    return MFRET_BACK;
}


static struct KeyValue color_setting[] = {
    { "daylight",    5600 },
    { "skylight",    6000 },
    { "flourescent", 4600 },
    { "tungsten",    3200 },
    { "light bulb",  2800 },
};

DEFUN(set_color_byname, 0)
{
    short i;

    for(i=0; i<ELEMENTSIN(color_setting); i++){
        if( !strcmp(color_setting[i].name, argv[1]) ){
            white_balance = color_setting[i].value;
            return MFRET_BACK;
        }
    }
    return MFRET_BACK;
}

DEFUN(set_disco_mode, 0)
{
    disco_mode = 1;
    return MFRET_BACK;
}

DEFUN(save_preset, 0)
{
}

extern const struct Menu guitop;

static void
lights_control(void){

    FILE *f = fopen("dev:oled0", "w");
    STDOUT = f;
    STDIN  = 0;
    int n = 0;
    int uln = 0;

    while(1){
        if( safety_override ){
            usleep(100000);
            continue;
        }

#if 1
        if( keypress ){
            switch(keypress){
            case 'm':
                menu( &guitop );
                n = 0;
                break;
            case 'a':
            case 'b':
                // load preset

            default:
                play(ivolume, "d-4>>");
                break;
            }
            keypress = 0;
        }
#else
        if( keypress ){
            if( keypress == '\a' ){
                play(ivolume, "d-4>>");
            }else{
                play(ivolume, "a5>>");

                switch(keypress){
                case '^':	power_level += 25;	break;
                case 'v':	power_level -= 25;	break;
                case '<':	white_balance -= 50;	break;
                case '>':	white_balance += 50;	break;
                case 'a':	white_balance = 5600;	break;
                case 'b':	white_balance = 3500;	break;
                case 'm':
                    power_level = 0;
                    white_balance = 4600;
                    pwm0 = pwm1 = 0;
                }

                if( power_level < 0 )   power_level = 0;
                if( power_level > MAX_PSET ) power_level = MAX_PSET;
                if( white_balance < 2700 ) white_balance = 2700;
                if( white_balance > 6500 ) white_balance = 6500;
            }
            keypress = 0;
        }
#endif

        if( n%10 == 0 ){
            int p = ((light_i0() + light_i1() - i0_off - i1_off) * light_vi() + 512) / 1024;
            int pe = p - power_level;
            pe = ABS(pe);

            bool ul  = 0;
            if( pe * 10 > power_level ){
                // batteries cannot deliver requested power
                if( uln ++ > 100 ) ul = 1;
            }else
                uln = 0;


            printf("\e[J");
            set_font("9x15");
            // reverse video on underload
            const char *ulrev = ul ? "\e[7m" : "";

            printf("power: %s%5d\e[27m\nwhite: %5d\n", ulrev, power_level, white_balance);
            set_font("5x8");
            printf("0 %5x %4d mA %5d mV\n", pwm0>>8, light_i0() - i0_off, light_v0());
            printf("1 %5x %4d mA %5d mV\n", pwm1>>8, light_i1() - i1_off, light_v1());
            printf("B %s%5d\e[27m %4d dC %s%5d mW\e[27m\n", ulrev, light_vi(), temp_max(), ulrev, p);
            printf("RGB   %0.06x %s\n", color_rgb(), (ul?"\e[7m*BATT*\e[27" : ""));
            if( ul ) play(ivolume, "c+6");

            n = 0;
        }
        n ++;
        usleep(10000);
    }
}

/****************************************************************/

// integral only control
// NB: the system response is dramatically different in ccm vs dcm
static inline int
adjust( int curr, int target, int i ){

    if( !target ) return 0;


    int err = (target - i);
    int inc = err / 64;
    int aer = (err < 0) ? - err : err;

    if( aer > 1 && aer < target / 2 ) inc /= 2;
    if( aer > 3 && aer < target / 4 ) inc /= 2;
    if( aer > 7 && aer < target / 8 ) inc /= 2;

    if( inc >  200 ) inc =  200;
    if( inc < -200 ) inc = -200;

    // system time constant ~ 150ms
    curr += (inc * 65536) / 105;	// 105 = 0.7 * Tc

    if( curr < 0 )       curr = 0;
    if( curr > MAX_PWM * 65536 ) curr = MAX_PWM * 65536;

    return curr;
}


static inline void
update_lights(void){

    // low pass filter
    i0f = (3 * i0f + 256 * (light_i0() - i0_off)) >> 2;
    i1f = (3 * i1f + 256 * (light_i1() - i1_off)) >> 2;

    // calc power levels
    // RSN - use color sensor
    int d = 6500 - 2700;
    int a = (256 * (white_balance - 2700) + d/2) / d;
    int vi = light_vi();
    int pt = (power_level * 1024 + vi/2) / vi;
    int t0 = a * pt;
    int t1 = (256 - a) * pt;

    pwm0 = adjust( pwm0, t0, i0f );
    pwm1 = adjust( pwm1, t1, i1f );

    pwm_set(   HWCF_TIMER_LEDS0,  pwm0 >> 16 );
    pwm_set(   HWCF_TIMER_LEDS1,  pwm1 >> 16 );

}

static void
lights_run(void){
    int n = 0, d = 0;

    FILE *f = fopen("dev:oled0", "w");
    STDOUT = f;
    STDIN  = 0;

    lights_offset();
    i0f = i1f = 0;
    pwm0 = pwm1 = 0;

    while(1){
        // read temp, color

        light_adc_read();
        read_temp_all();
        read_color_all();
        safety_check();
        check_power_switch();

        int c = read_touch_all();
        if( c ){
            keypress = c;
        }

        if( disco_mode ){
            if( n%500 == 0 ){
                if( white_balance == 2700 )
                    white_balance = 6500;
                else
                    white_balance = 2700;
            }
            n ++;
        }

        update_lights();
        usleep(1000);
    }
}

void
lights_start(void){

    start_proc(2048, lights_control, "ledctl");
    start_proc(2048, lights_run,     "ledrun");
}

struct LogDat {
    int p;
    unsigned short i, v;
};
static struct LogDat datbuf[2048];

DEFUN(testlight, "test leds")
{

    if( argc != 2 ) return 0;

    power_level   = atoi( argv[1] );

    lights_offset();
    i0f = i1f = 0;
    pwm0 = pwm1 = 0;

    int n = 0;

    while(1){
        light_adc_read();
        safety_check();

        if( check_button() ) break;
        if( n > 100000 )     break;

        update_lights();

        //if( (n % 100) == 0 ){
        //    printf("p0 %6d p1 %6d; 0[%4d %6d %5d] 1[%4d %6d %5d]\n",
        //           pwm0 >> 8, pwm1 >> 8,
        //           light_i0() - i0_off, i0f>>8, light_v0(),
        //           light_i1() - i1_off, i1f>>8, light_v1() );
        //}

        if( n < ELEMENTSIN(datbuf) ){
            datbuf[n].i = i0f;
            datbuf[n].v = light_v0();
            datbuf[n].p = pwm0 >> 8;
        }

        n ++;
        usleep(1000);
    }

    lights_safe();

    for(n=0; n<2048; n++){
        printf("%d %d %d\n", datbuf[n].p, datbuf[n].i, datbuf[n].v);
    }


    return 0;
}

DEFUN(testpwm, "test leds")
{
    int pwm0=0, pwm1=0;
    int n = 0;

    if( argc != 2 ) return 0;

    lights_offset();

    utime_t t0 = get_time();

    pwm0 = pwm1 = atoi( argv[1] );
    pwm_set(   HWCF_TIMER_LEDS0,  pwm0 );
    pwm_set(   HWCF_TIMER_LEDS1,  pwm1 );

    while(1){
        utime_t t1 = get_time();
        light_adc_read();
        safety_check();

        if( check_button() ) break;
        if( ++n > 500 )     break;

        printf("%6d: p0 %6d i0 %6d v0 %6d; p1 %6d i1 %6d v1 %6d\n",
               (int)(t1 - t0),
               pwm0, light_i0() - i0_off, light_v0(), pwm1, light_i1() - i1_off, light_v1() );

    }

    lights_safe();

    return 0;
}

