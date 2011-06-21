/* This file is part of rfBareMbed.                        
 *									 
 * rfBareMbed is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by	 
 * the Free Software Foundation, either version 3 of the License, or	 
 * (at your option) any later version.					 
 * 									 
 * rfBareMbed is distributed in the hope that it will be useful,		 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of	 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	 
 * GNU General Public License for more details.				 
 * 									 
 * You should have received a copy of the GNU General Public License	 
 * along with rfBareMbed.  If not, see <http://www.gnu.org/licenses/>.	 
 */

#include <stdint.h>
#include "drivers/leds.h"

#include "clock.h"
#include "interrupt.h"
#include "debug.h"
#include "config.h"

/* These symbols are defined by the linker script.
   They are used to find bounds of text, data and bss sections in flash/ram
*/
extern char _data_start;
extern char _data_end;
extern char _text_end;
extern char _bss_start;
extern char _bss_end;

/* reset routine */
void _start(void);
/* default handlers */
extern LPC_IRQ_HANDLER _default_exception_handler();
extern LPC_IRQ_HANDLER _default_peripheral_handler();


/** This array will contain the stack. It is forced to be in the .stack section
 * so that the linker script can locate it where we want */
static unsigned char _stack[STACK_SIZE] __attribute__((section(".stack")));

/** This array is the rom interrupt vector. It is force to be located in the
 * .interrupt_vector section so that the linker script puts it at address 0 */

/* see section 34.3.3 p 745 of the user manual */

void* _rom_interrupts[IRQn_COUNT] __attribute__ ((section(".interrupt_vector"))) = {
    &_stack[STACK_SIZE],   /* initial SP value */
    _start, /* address of the reset routine */
    _default_exception_handler,	/* Non Masquable Interrupt handler, */
    _default_exception_handler,	/* Hardware Fault handler, */
    _default_exception_handler,	/* Memory protection unit exception handler, */
    _default_exception_handler,	/* Bus fault handler, */
    _default_exception_handler,	/* Usage fault handler, */
    0,			/* Reserved */
    0,			/* Reserved */
    0,			/* Reserved */
    0,			/* Reserved */
    _default_exception_handler,	/* Supervisor mode call handler, */
    0,			/* Reserved */
    0,			/* Reserved */
    _default_exception_handler,	/* pend sv handler (c.f. p746 of user manual) */
    _default_exception_handler,	/* system timer tick handler (generated when the timer reaches 0) */
/* From	now, all interrupts are external IRQ for peripherals */
    _default_peripheral_handler,	/* Watchdog timer */
    _default_peripheral_handler,	/* Timer0 */
    _default_peripheral_handler,	/* Timer1 */
    _default_peripheral_handler,	/* Timer2 */
    _default_peripheral_handler,	/* Timer3 */
    _default_peripheral_handler,	/* UART0 */
    _default_peripheral_handler,	/* UART1 */
    _default_peripheral_handler,	/* UART2 */
    _default_peripheral_handler,	/* UART3 */
    _default_peripheral_handler,	/* PWM1 */
    _default_peripheral_handler,	/* I2C0 */
    _default_peripheral_handler,	/* I2C1 */
    _default_peripheral_handler,	/* I2C2 */
    _default_peripheral_handler,	/* SPI */
    _default_peripheral_handler,	/* SSP0 */
    _default_peripheral_handler,	/* SSP1 */
    _default_peripheral_handler,	/* PLL0 Lock */
    _default_peripheral_handler,	/* Real Time Clock */
    _default_peripheral_handler,	/* External Interrupt 0 */
    _default_peripheral_handler,	/* External Interrupt 1 */
    _default_peripheral_handler,	/* External Interrupt 2 */
    _default_peripheral_handler,	/* External Interrupt 3 */
    _default_peripheral_handler,	/* Analogic Digital Converter */
    _default_peripheral_handler,	/* Brown-Out detect */
    _default_peripheral_handler,	/* USB */
    _default_peripheral_handler,	/* CAN */
    _default_peripheral_handler,	/* General Purpose DMA */
    _default_peripheral_handler,	/* I2S */
    _default_peripheral_handler,	/* Ethernet */
    _default_peripheral_handler,	/* Repetitive Interrupt Timer */
    _default_peripheral_handler,	/* Motor Control PWM */
    _default_peripheral_handler,	/* Quadrature Encoder */
    _default_peripheral_handler,	/* PLL1 Lock */	
};


/* Set all bss to zero */
static void _zero_bss(void)
{
    char *dst = &_bss_start;
    while (dst < &_bss_end)
	*dst++ = 0;
}

/* Copy data section from flash to ram */
static void _copy_data_section(void)
{
    char *src = &_text_end;
    char *dst = &_data_start;

    while (dst < &_data_end)
	*dst++ = *src++;
}



extern void main(void);

void _start(void)
{
    lpc_init_leds();
    _copy_data_section();
    _zero_bss();
    /* set the clock to 96 Mhz */
    rflpc_init_clock();
    /* Copy the rom interrupt vector to ram and relocate it */
    lpc_init_interrupts();
    /* start main program */
    main();
}
