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

// LCD nibble send.
static void lcdns(uint8_t val, uint8_t RS)
{
	val = (val&0xF) | (1<<4u) | (RS<<5U);
	PORTL = val;
	_delay_ms(2);
	PORTL &= ~(1u<<4u);
}

// byte
static void lcdb(uint8_t val, uint8_t RS)
{
	lcdns(val>>4u, RS);
	lcdns(val, RS);
}

static void FillLine()
{
for (unsigned char c = 'A'; c!='U'; c++)
	lcdb(c,1);
}

static void FillLine2()
{
for (unsigned char c = 'a'; c!='u'; c++)
	lcdb(c,1);
}

static void FillLineN()
{
for (unsigned char c = '0'; c!=':'; c++)
	lcdb(c,1);
}

int main()
{
	stdout = &mystdout;
	DDRL=0xFF;
	sei();
	// Setup 4bit mode:
	for (int i=0; i<3; i++)
		lcdns(0b0011,0);
	lcdns(0b0010,0);
	lcdb(0x28,0); // 2 lines;
	lcdb(0x06,0); // 2 lines;

	lcdb(0x80,0); // ddr 0.
	printf("READY\n");
	// write line 1.
	FillLine();

	printf("L1\n");

	lcdb(0x02,0);
	lcdb('Z',1);
	printf("HOME\n");

	lcdb(0x01,0);

	printf("L1C\n");

	lcdb(0xC0,0); // ddr 40.
	// write line 1.
	FillLine();
	printf("L2\n");

	lcdb(0x01,0);
	printf("L2C\n");

	lcdb(0x8A,0); // ddr pos 10.
	FillLine();
	FillLine2();
	FillLine();
	FillLineN();
	FillLineN();
	printf("overrun\n");

	lcdb(0x01,0);
	lcdb(0x04,0); // RTL
	lcdb(0x89,0); // ddr pos 10.

	FillLine();
	FillLine2();
	FillLineN();
	FillLineN();
	printf("RTL\n");

	lcdb(0x06,0); // LTR
	lcdb(0x84,0); // ddr pos 10.
	lcdb('\t',1);
	printf("CGR\n");

	// Write some stuff to cgram
	lcdb(0x40,0);
	for (int i=0; i<66; i++)
		lcdb(i,1);

	printf("CGR2\n");

	lcdb(0x04,0); // LTR
	// Write some stuff to cgram
	lcdb(0x44,0);
	for (int i=0; i<66; i++)
		lcdb(i,1);

	printf("CGR3\n");

	while(1){};

	cli();


	printf("FINISHED\n");

	// this quits the simulator, since interupts are off
	// this is a "feature" that allows running tests cases and exit
	sleep_cpu();
}
