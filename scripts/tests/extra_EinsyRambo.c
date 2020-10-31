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

ISR(USART0_RX_vect)
{
	//uint8_t b = UDR0;
	//done++;
//	sleep_cpu();
}

// LCD nibble send.
static void lcdns(uint8_t val, uint8_t RS)
{
	PORTF |= 1<<7;
	if (val&1)
		PORTF |= 1<<5;
	else
		PORTF &= ~(1<<5);
	if (RS&1)
		PORTD |= 1<<5;
	else
		PORTD &= ~(1<<5);
	if (val&2)
		PORTG |= 1<<4;
	else
		PORTG &= ~(1<<4);
	if (val&4)
		PORTH |= 1<<7;
	else
		PORTH &= ~(1<<7);
	if (val&8)
		PORTG |= 1<<3;
	else
		PORTG &= ~(1<<3);
	_delay_ms(2);

	PORTF &= ~(1u<<7u);
}

// byte
static void lcdb(uint8_t val, uint8_t RS)
{
	lcdns(val>>4u, RS);
	lcdns(val, RS);
}

static const char strDisp[] = "                       Prusa Research    Original Prusa i3";

static const char strClk[] = " Click";

int main()
{
	stdout = &mystdout;
	DDRD = 1 <<5; // RS
	DDRG = 3 <<3; // D5/D7
	DDRH = 1 <<7; // D6
	DDRF = 1<<7 | 1<<5; //EN/D4
	sei();

	for (int i=0; i<3; i++)
		lcdns(0b0011,0);
	lcdns(0b0010,0);
	lcdb(0x28,0); // 2 lines;
	lcdb(0x06,0); // 2 lines;

	lcdb(0x80,0); // ddr 0.

	for (int i=0; i<59; i++)
		lcdb(strDisp[i],1);

	printf("READY\n");

	while (PINH & 1<<6)
	{
		_delay_ms(10);
	}
	DDRG |= 1<<5;

	for (int i=0; i<7; i++)
		lcdb(strClk[i],1);
	printf("BED\n");


	while (1)
	{
		_delay_ms(100);
	};

	cli();

	printf("FINISHED\n");

	// this quits the simulator, since interupts are off
	// this is a "feature" that allows running tests cases and exit
	sleep_cpu();
}
