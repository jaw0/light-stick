/*
  Copyright (c) 2013
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2013-Apr-18 20:28 (EDT)
  Function: menus, accelerometer for user input
*/


#include <conf.h>
#include <proc.h>
#include <gpio.h>
#include <msgs.h>
#include <pwm.h>
#include <sys/types.h>
#include <ioctl.h>
#include <userint.h>

#include "uimenu.h"

#define IDLETIME	5000000		// usecs until screensaver

// XXX - assumes 128x64 oled display

/*
u+80	arrowleft
u+81	arrowup
u+82	arrowright
u+83	arrowdown
u+84	arrowupdn
u+85	arrowboth
*/

extern void beep(int, int, int);
extern int ivolume;
extern int keypress;

#define set_leds_rgb(a,b)

/****************************************************************/


static clear_line(int dir){
    int i;

    for(i=0; i<26; i++){
        if( dir > 0 )
            printf("\e[<");
        else
            printf("\e[>");
        printf("\xB");
        usleep(10000);
    }
}

static int
get_input(void){
    int c = 0;

    utime_t t0 = get_time();

    while( !c ){

        if( get_time() - t0 >= IDLETIME ) return 0;

        // button = enter
        if( check_button() ){
            beep(400, ivolume, 10000);
            c = '\n';
            break;
        }

        c = keypress;
        keypress = 0;

        if( c == '\a' ){
            beep(250, ivolume, 50000);
            c = 0;
        }else if( c )
            beep(400, ivolume, 10000);
        else
            usleep(10000);
    }

    return c;
}

static inline void
topline(const char *title, int nopts){

    // display in big font, top line: "title (count)     arrow"
    // RSN - color
    printf("\e[J\e[16m\e[4m%s(%d)\e[24m\e[-1G\x84\n\e[16m", title, nopts);
}

static inline void
botline(const char *foot){
    if(!foot) return;
    // 5x8 font at bottom
    // RSN - color
    // RSN - no footer on small display
    printf("\e[13m\e[-1;0H%s\e[16m\e[1;0H", foot);
}

// return:
//   menu | MFRET_*

static const struct MenuOption *
domenu(const struct Menu *m){
    int nopt  = 0;
    int nopts = 0;
    int ch;

    // count options
    for(nopts=0; m->el[nopts].type; nopts++) {}

    topline(m->title, nopts);

    if( m->startval ) nopt = * m->startval;

    while(1){

        // display current choice, bottom line: "choice     arrow"
        printf("\e[0G%s\e[-1G%c\xB", m->el[nopt].text,
               (nopt==0 && nopts==1) ? ' ' :
               (nopt==0) ? '\x82' :
               (nopt==nopts-1) ? '\x80' : '\x85'
            );

        ch = get_input();
        if( !ch ) return MFRET_IDLE;

        switch(ch){
        case '?':
            // refresh screen
            topline(m->title, nopts);
            break;
        case '<':
            nopt --;
            if( nopt < 0 ){
                nopt = 0;
                beep(200, ivolume, 500000);
            }
            clear_line(-1);
            break;
        case '>':
            nopt ++;
            if( nopt > nopts - 1 ){
                nopt = nopts - 1;
                beep(200, ivolume, 500000);
            }
            clear_line(1);
            break;
        case '^':
            return (void*)MFRET_BACK;
        case 'm':
            return (void*)MFRET_MENU;
        case '\n':
            return (void*) & m->el[nopt];
        default:
            beep(200, ivolume, 50000);
            break;
        }
    }
}



void
menu(const struct Menu *m){
    const struct Menu *main = m;
    Catchframe cf;
    const struct MenuOption *opt;
    const void *r;

    // catch ^C (or equiv)
    if(0){
    xyz:
        UNCATCH(cf);
        set_leds_rgb( 0xFF0000, 0xFF0000 );
        play(ivolume, "D+3>D-3>");
        set_leds_rgb( 0, 0 );
    }
    CATCHL(cf, MSG_CCHAR_0, xyz);

    while(1){
        if( !m ) return;

        opt = domenu(m);

        switch( (int)opt){
        case MFRET_IDLE:
            return;
        case MFRET_BACK:
            m = m->prev;
            break;
        case MFRET_MENU:
            m = main;
            break;
        case 0:
            // stay here
            break;
        default:
            switch(opt->type){
            case MTYP_EXIT:
                UNCATCH(cf);
                return;
            case MTYP_MENU:
                r = opt->action;
                break;
            case MTYP_FUNC:
                printf("\e[10m\e[J");
                r = ((void*(*)(int,const char**,void*))opt->action)(opt->argc, opt->argv, 0);
                break;
            }
            switch((int)r){
            case MFRET_BACK:
                m = m->prev;
                break;
            case MFRET_IDLE:
                return;
            case MFRET_MENU:
                m = main;
                break;
            default:
                m = (const struct Menu*)r; // sub-menu
                break;
            }

            break;
        }
    }

    UNCATCH(cf);
}

/****************************************************************/

/* return user entered number */
int
menu_get_int(const char *prompt, const char *foot, int min, int max, int inc, int *value){
    int nopt  = *value;
    int nopts = (max - min) / inc + 1;

    topline(prompt, nopts);
    botline(foot);

    while(1){
        // display current choice, bottom line: "choice     arrow"
        printf("\e[0G%d\e[99G%c\xB", nopt,
               (nopt==0 && nopts==1) ? ' ' :
               (nopt==0) ? '\x82' :
               (nopt==nopts-1) ? '\x80' : '\x85'
            );

        // get input
        int ch = get_input();

        switch(ch){
        case 0:
            return MFRET_IDLE;
        case 'm':
            return MFRET_MENU;
        case '<':
            return MFRET_BACK;
        case '?':
            topline(prompt, nopts);
            botline(foot);
            break;
        case 'v':
            nopt -= inc;
            if( nopt < min ){
                nopt = min;
                beep(200, ivolume, 500000);
            }
            *value = nopt;
            clear_line(-1);
            break;
        case '^':
            nopt += inc;
            if( nopt > max ){
                nopt = max;
                beep(200, ivolume, 500000);
            }
            *value = nopt;
            clear_line(1);
            break;

        case '\n':
            return nopt;

        default:
            beep(200, ivolume, 50000);
            break;

        }
    }
}

