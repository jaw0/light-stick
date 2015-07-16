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
#include <userint.h>

#include "util.h"


//#define CRUMBS  "adc"
#include "crumbs.h"
//#define DROP_CRUMB(a,b,c)
//#define RESET_CRUMBS()
//#define DUMP_CRUMBS()

#define DMAC		DMA2_Stream0

#define SR_EOC		2
#define SR_OVR		(1<<5)
#define CR2_SWSTART	0x40000000
#define CR2_ADON	1

#define DMASCR_MINC	(1<<10)
#define DMASCR_DIR_M2P  (1<<6)
#define DMASCR_PFCTRL	(1<<5)
#define DMASCR_TEIE	(1<<2)
#define DMASCR_TCIE	(1<<4)
#define DMASCR_HIPRIO	(3<<16)
#define DMASCR_MEM32	(2<<13)
#define DMASCR_DEV32	(2<<11)
#define DMASCR_MEM16	(1<<13)
#define DMASCR_DEV16	(1<<11)
#define DMASCR_EN	1

static short dmabuf[16];

static void adc_dma_sqr_init(void);
static void adc_dma_get_all(void);

static lock_t adclock;
static short adcstate;
static short adc_count;
#define ADC_DONE	 0
#define ADC_BUSY	 1
#define ADC_ERROR	-1


/****************************************************************/

void
light_adc_init(void){
    adc_dma_sqr_init();
}

void
light_adc_read(void){
    adc_dma_get_all();
}


// convert to Volts/Amps, Q.10

int
light_i0(void){	// A2
    // I = A * 3.3 / 4096 * 100 * 0.01
    return (dmabuf[1] * 33) / 40;
}
int
light_i1(void){	// A5
    return (dmabuf[4] * 33) / 40;
}
int
light_v0(void){	// A3
    // V = A * (88k7 + 3k3) / 3k3 * 3.3 / 4096
    return (dmabuf[2] * 92) >> 2;
}
int
light_v1(void){	// A4
    return (dmabuf[3] * 92) >> 2;
}
int
light_vi(void){	// A0
    // V = A * (14k7 + 3k3) / 3k3 * 3.3 / 4096
    return (dmabuf[0] * 18) >> 2;
}

DEFUN(testadc, "test adc")
{
    while(1){
        light_adc_read();
        printf("bv: %6d\n", light_vi() );
        printf("i0: %6d  v0: %6d\n", light_i0(), light_v0());
        printf("i1: %6d  v1: %6d\n", light_i1(), light_v1());

        printf("\n");

        if( check_button() ) break;
        usleep(250000);
    }
    return 0;
}

/****************************************************************/

static inline void
_enable_dma(int cnt){

    DMAC->CR    &= ~1;		// clear EN first
    DMA2->LIFCR |= 0x3D;	// clear irq

    // wait for it to disable
    while( DMAC->CR & 1 )
        ;

    DMAC->CR   = 0;

    DMAC->FCR   = DMAC->FCR & ~3;
    DMAC->FCR  |= 1<<2;
    DMAC->PAR   = (u_long) & ADC1->DR;
    DMAC->M0AR  = (u_long) & dmabuf;
    DMAC->NDTR  = cnt;

    DMAC->CR   |= DMASCR_TEIE | DMASCR_TCIE | DMASCR_MINC | DMASCR_HIPRIO | DMASCR_MEM16 | DMASCR_DEV16
        | (0<<25)	// channel 0
        ;

    ADC1->CR2 |= 1<<8; // DMA
    DMAC->CR  |= 1;     // enable

    DROP_CRUMB("ena dma", DMA2->LISR, ADC1->SR);

}

static inline void
_disable_dma(void){

    DROP_CRUMB("dis dma", DMA2->LISR,DMAC->NDTR);

    ADC1->CR2 &= ~(1<<8);
    DMAC->CR &= ~(DMASCR_EN | DMASCR_TEIE | DMASCR_TCIE);
    DMA2->LIFCR |= 0x3D;	// clear irq
}

static void
adc_dma_init(void){

    DROP_CRUMB("dma init", 0, 0);
    ADC1->CR1    |= 1<<8;	// scan mode
    RCC->AHB1ENR |= 1<<22;	// DMA2
    nvic_enable( IRQ_DMA2_STREAM0, 0x40 );
}

static void
adc_dma_sqr_init(void){

    adc_dma_init();
}

static void
adc_dma_get_all(void){

    //sync_lock( &adclock, "adc.L" );

    adc_count = 5;

    ADC1->SR = ADC1->SR & ~0x3f;

    RESET_CRUMBS();
    DROP_CRUMB("adc start", ADC1->SR, adc_count);
    bzero(dmabuf, sizeof(dmabuf));

    ADC1->SQR3 = (0) | (2 << 5) | (3 << 10) | (4 << 15) | (5 << 20);
    ADC1->SQR1 = (adc_count - 1) << 20;

    _enable_dma(adc_count);
    adcstate = ADC_BUSY;
    ADC1->CR2 |= CR2_SWSTART;	// Go!
    DROP_CRUMB("adc started", ADC1->SR, DMA2->LISR);

    utime_t to = get_hrtime() + 10000;
    while(1){
        asleep( &adc_dma_get_all, "adc");
        if( adcstate != ADC_BUSY ) break;
        await( -1, 1000 );
        if( adcstate != ADC_BUSY ) break;
        if( get_hrtime() >= to ){
            DROP_CRUMB("toed", err, ADC1->SR);
            adcstate = ADC_ERROR;
        }
    }
    aunsleep();

    _disable_dma();

    if( adcstate == ADC_ERROR ){
        printf("adc error\n");
    }
    DROP_CRUMB("adc done", 0,0);
    //DUMP_CRUMBS();

    //sync_unlock( &adclock );
}

void
DMA2_Stream0_IRQHandler(void){
    int isr = DMA2->LISR & 0x3F;
    DMA2->LIFCR |= 0x3D;	// clear irq

    DROP_CRUMB("dmairq", isr, 0);
    if( isr & 8 ){
        // error - done
        _disable_dma();
        adcstate = ADC_ERROR;
        DROP_CRUMB("dma-err", 0,0);
        wakeup( &adc_dma_get_all );
        return;
    }

    if( isr & 0x20 ){
        // dma complete
        if( ((u_long)dmabuf == DMAC->M0AR) && ! DMAC->NDTR ){
            _disable_dma();
            adcstate = ADC_DONE;
            DROP_CRUMB("dma-done", 0,0);
            wakeup( &adc_dma_get_all );
        }
    }
}
