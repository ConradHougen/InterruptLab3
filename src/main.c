/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC11xx.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>

// TODO: insert other include files here

// definitions and declarations
#define CONFIG_GPIO_DEFAULT_PIOINT2_IRQHANDLER 1
// 1ms timer interval
#define TIMER_INTERVAL (SystemCoreClock/1000 - 1)
// LED bit is 7
#define LED 7

// definitions for GPIO registers
#define GPIO0_DIR ((int *)(0x50008000UL))
//#define GPIO0_DATA ((int *)(0x50000000UL))
#define GPIO2_IS ((int *)0x50028004UL)
#define GPIO2_IBE ((int *)0x50028008UL)
#define GPIO2_IEV ((int *)0x5002800CUL)
#define GPIO2_IE ((int *)0x50028010UL)
#define GPIO2_IC ((int *)0x5002801CUL)

// definitions for timer0 registers
#define SYSCON_SYSAHBCLKCTRL ((int *)0x40048080UL)
#define IOCON_PIO1_5 ((int*)0x400440A0UL)
#define IOCON_PIO1_6 ((int*)0x400440A4UL)
#define IOCON_PIO1_7 ((int*)0x400440A8UL)
#define IOCON_PIO0_1 ((int*)0x40044010UL)
#define TMR32B0_MR0 ((int *)0x40014018UL)
#define TMR32B0_CCR ((int *)0x40014028UL)
#define TMR32B0_MCR ((int *)0x40014014UL)
#define TMR32B0_TCR ((int *)0x40014004UL)
#define TMR32B0_IR ((int *)0x40014000UL)


volatile static uint32_t period;
volatile static double duty;
volatile uint32_t timer32_0_counter = 0;
volatile uint32_t timer32_0_capture = 0;
volatile uint32_t timer_val = 0;

static double duty_array[] = {0.25, 0.75};
static int mode = 0;

/* GPIO and GPIO Interrupt Initialization */
void GPIOInit() {

	/* set edge triggering */
	*GPIO2_IS &= ~(0x1<<1);
	/* single edge */
	*GPIO2_IBE &= ~(0x1<<1);
	/* active high */
	*GPIO2_IEV &= ~(0x1<<1);
	/* enable interrupt on pin 1 */
	*GPIO2_IE |= (0x1<<1);


	/* Set priority of gpio2 interrupt to 0 */
	NVIC_SetPriority(EINT2_IRQn, 0);
}

/* TIMER32 and TIMER32 Interrupt Initialization */
void TIMERInit() {

	*SYSCON_SYSAHBCLKCTRL |= (1<<9);
	*IOCON_PIO1_5 &= ~0x07;	/*  Timer0_32 I/O config */
	*IOCON_PIO1_5 |= 0x02;	/* Timer0_32 CAP0 */
	*IOCON_PIO1_6 &= ~0x07;
	*IOCON_PIO1_6 |= 0x02;	/* Timer0_32 MAT0 */
	*IOCON_PIO1_7 &= ~0x07;
	*IOCON_PIO1_7 |= 0x02;	/* Timer0_32 MAT1 */
	*IOCON_PIO0_1 &= ~0x07;
	*IOCON_PIO0_1 |= 0x02;	/* Timer0_32 MAT2 */

	// init counter and capture registers
	timer32_0_counter = 0;
	timer32_0_capture = 0;

	// timer interval determines rate
	*TMR32B0_MR0 = TIMER_INTERVAL;
	/* Capture 0 on rising edge, interrupt enable. */
	*TMR32B0_CCR = (0x1<<0)|(0x1<<2);
	/* Interrupt and Reset on MR0 */
	*TMR32B0_MCR = 3;



	/* Set priority of timer0 interrupt to 0 */
	NVIC_SetPriority(TIMER_32_0_IRQn, 0);

}

/* GPIO Interrupt Handler */
void PIOINT2_IRQHandler(void) {
	// period in ticks
	period = (timer32_0_counter - timer_val);

	// record last count value
	timer_val = timer32_0_counter;

	// clear the interrupt on pin 1
	*GPIO2_IC |= (0x1<<1);
}

/* TIMER32 Interrupt Handler */
void TIMER32_0_IRQHandler(void) {
	if ( *TMR32B0_IR & 0x01 )
	{
		*TMR32B0_IR = 1;				/* clear interrupt flag */
		timer32_0_counter++;
	}
	if ( *TMR32B0_IR & (0x1<<4) )
	{
		*TMR32B0_IR = 0x1<<4;			/* clear interrupt flag */
		timer32_0_capture++;
	}

	// do we need to change duty cycle? (change every 10000 ticks)
	if( (timer32_0_counter % 10000) == 0 )
	{
		mode = !mode;
		duty = duty_array[mode];
	}

	// toggle LED?
	if( (timer32_0_counter % period) == (int)(period*duty) )
	{
		// LED should go low
		LPC_GPIO0->MASKED_ACCESS[(1<<LED)] = (0<<LED);
	}
	else if( (timer32_0_counter % period ) == 0 )
	{
		// LED should go high
		LPC_GPIO0->MASKED_ACCESS[(1<<LED)] = (1<<LED);
	}

	return;
}

int main(void) {

    /* Initialization code */
	duty = duty_array[mode];
	// set LED to output direction
	//LPC_GPIO0->DIR |= 1<<LED;
	*GPIO0_DIR |= 1<<LED;
	// start LED low
	LPC_GPIO0->MASKED_ACCESS[(1<<LED)] = (0<<LED);

    GPIOInit();                   // Initialize GPIO ports for both Interrupts and LED control
    TIMERInit();                // Initialize Timer and Generate a 1ms interrupt

    /* start counting */
    *TMR32B0_TCR = 1;
    /* Enable the TIMER0 Interrupt */
    NVIC_EnableIRQ(TIMER_32_0_IRQn);
    NVIC_EnableIRQ(EINT2_IRQn); //enable gpio interrupt




    /* Infinite looping */
    while(1);


    return 0;
}
