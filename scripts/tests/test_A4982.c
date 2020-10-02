/*
	atmega88_uart_echo.c

	This test case enables uart RX interupts, does a "printf" and then receive characters
	via the interupt handler until it reaches a \r.

	This tests the uart reception fifo system. It relies on the uart "irq" input and output
	to be wired together (see simavr.c)
 */

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <util/delay.h>

/*
 * This demonstrate how to use the avr_mcu_section.h file
 * The macro adds a section to the ELF file with useful
 * information for the simulator
 */
#include "avr_mcu_section.h"
AVR_MCU(16000000, "atmega2560");
// tell simavr to listen to commands written in this (unused) register
// AVR_MCU_SIMAVR_COMMAND(&GPIOR0);
// AVR_MCU_SIMAVR_CONSOLE(&GPIOR1);

/*
 * This small section tells simavr to generate a VCD trace dump with changes to these
 * registers.
 * Opening it with gtkwave will show you the data being pumped out into the data register
 * UDR0, and the UDRE0 bit being set, then cleared
 */
// const struct avr_mmcu_vcd_trace_t _mytrace[]  _MMCU_ = {
// 	{ AVR_MCU_VCD_SYMBOL("UDR3"), .what = (void*)&UDR3, },
// 	{ AVR_MCU_VCD_SYMBOL("UDRE3"), .mask = (1 << UDRE3), .what = (void*)&UCSR3A, },
// 	{ AVR_MCU_VCD_SYMBOL("GPIOR1"), .what = (void*)&GPIOR1, },
// };
// #ifdef USART3_RX_vect_num	// stupid ubuntu has antique avr-libc
// AVR_MCU_VCD_IRQ(USART3_RX);	// single bit trace
// #endif
// AVR_MCU_VCD_ALL_IRQ();		// also show ALL irqs running

#define PIN(x,y) PORT##x>>y & 1U

volatile uint8_t done = 0;

static int uart_putchar(char c, FILE *stream)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                         _FDEV_SETUP_WRITE);

void step()
{
	PORTA |= 1u<<2;
	_delay_ms(1);
	PORTA&=~(1U<<2);
	_delay_ms(1);
}

int main()
{
	stdout = &mystdout;
	sei();

	DDRA = 0b00011101; // TMC control lines.
	DDRJ = 0b11110000;

	printf("READY\n");

	for(int i=0; i<10; i++)
		step();

	printf("RESETSTEP\n");
	PORTJ|=0x10; // RST high

	PORTJ|=0x80; // Sleep high

	for(int i=0; i<10; i++)
		step();
	printf("SLEEPSTEP\n");

	_delay_ms(5);
	PORTA|=(1U<<4);
	for(int i=0; i<10; i++)
		step();

	printf("DISABLED\n");
	PORTA&=~(1U<<4);

	_delay_ms(10);

	PORTJ&=0x7F; // SLP low.
	_delay_ms(5);


	int i=0;
	while(!(PINJ&0b1000))
	{
		step();
		i++;
	}

	printf("ST CT %d\n",i);
	_delay_ms(5);
	PORTA|=(1U<<3); // DIR
	i--;
	step();
	while(!(PINJ&0b1000))
	{
		step();
		i--;
	}
	printf("ST CT %d\n",i);

	PORTA&=~(1U<<3); // DIR
	PORTJ |=0x40; // MS 8

	for (int i=0; i<64; i++)
		step();

	printf("STEP64 8\n");

	PORTJ&= ~0x40; // MS 8
	PORTJ |= 0x20;

	for (int i=0; i<64; i++)
		step();

	printf("STEP64 4\n");

	PORTJ |= 0x60;
	for (int i=0; i<64; i++)
		step();

	_delay_ms(25);
	printf("STEP64 1\n");

	while(1);

	cli();

	printf("FINISHED\n");

	// this quits the simulator, since interupts are off
	// this is a "feature" that allows running tests cases and exit
	sleep_cpu();
}
