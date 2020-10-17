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

uint8_t SPI_TX(uint8_t cData){

   SPDR = cData;
   while(!(SPSR & (1<<SPIF)))
      ;
   return SPDR;
}

uint8_t uiStatus;
unsigned long uiReply;

void TMCTX(uint8_t uiAddr, unsigned long uiData)
{
	PORTA &= 0xFE;
	uiReply =0;
	uiStatus = SPI_TX(uiAddr);
	for (unsigned int i=0; i<4; i++)
	{
		uint8_t out = (uiData >> (8U*(3U-i)) & 0xFF);
		uiReply |= SPI_TX(out)<<(8U*i);
	}
	PORTA|=1;
}

void step()
{
	PORTA |= 1u<<2;
	_delay_ms(1);
	PORTA&=~(1U<<2);
}

int main()
{
	stdout = &mystdout;
	sei();

	DDRB = ((1<<DDB2)|(1<<DDB1)); //spi pins on port b MOSI SCK,SS outputs
  	SPCR = ((1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<CPOL)|(1<<CPHA));  // SPI enable, Master, f/16
	DDRA = 0b00011101; // TMC control lines.

	printf("READY\n");

	TMCTX(0x6F,0);

	_delay_ms(5);

	TMCTX(0x01,0);

	printf("RP %02x %06lx\n",uiStatus, uiReply);

	TMCTX(0x6F,0);

	printf("RP %02x %06lx\n",uiStatus, uiReply);

	TMCTX(0x80,1U<<7U); // DIAG0_stall

	printf("DIAG %02x\n",PINA&0x02);

	TMCTX(0x80,1U<<7U | 1U << 12); // DIAG0_stall && pushpull

	printf("DIAG %02x\n",PINA&0x02);

	for(int i=0; i<160; i++)
	{
		step();
		if (PINA&0x2)
			printf("DIAG ERR\n");
	}

	step();

	printf("DIAGMAX %02x\n",PINA&0x02);

	// Reverse dir:
	PORTA|=(1U<<3);

	for(int i=0; i<320; i++)
	{
		step();
		if (PINA&0x2)
			printf("DIAG ERR\n");
	}

	step();

	printf("DIAGMIN %02x\n",PINA&0x02);

	// try DEDGE
	TMCTX(0x6C|0x80, 0x20000000);

	PORTA&=~(1U<<3);
	for(int i=0; i<160; i++)
	{
		step();
		if (PINA&0x2)
			printf("DIAG ERR\n");
	}

	PORTA |= 1u<<2;
	printf("DIAGDEDGE %02x\n",PINA&0x02);

	PORTA|=(1U<<3); // rev dir
	PORTA&=~(1U<<2); // step.

	PORTA&=~(1U<<3);

	// DISABLE
	PORTA|=(1U<<4);
	printf("DISABLED\n");
	if (PINA&0x2)
			printf("DIAG NOT CLEARED\n");

	for(int i=0; i<10; i++)
	{
		step();
		if (PINA&0x2)
			printf("ERR STEP WHILE DISABLED\n");
	}
	PORTA&=~(1U<<4);
	step();
	printf("DIAGDISABLE %02x\n",PINA&0x02);

	TMCTX(0x80,1U<<7U); // DIAG0_stall, Act low

	printf("DIAG %02x\n",PINA&0x02);


	cli();


	printf("FINISHED\n");

	// this quits the simulator, since interupts are off
	// this is a "feature" that allows running tests cases and exit
	sleep_cpu();
}
