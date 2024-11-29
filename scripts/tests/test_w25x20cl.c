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
int main()
{
	stdout = &mystdout;
	sei();

	DDRB = ((1<<DDB2)|(1<<DDB1)|(1<<DDB0)); //spi pins on port b MOSI SCK,SS outputs
  	SPCR = ((1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<CPOL)|(1<<CPHA));  // SPI enable, Master, f/16

	PORTB&=0b11111110; // set CS low.
	DDRJ = 0x00;

	printf("READY\n");

	SPI_TX(0x90);
	SPI_TX(0x00);
	SPI_TX(0x00);
	printf("SPI %02x\n", SPI_TX(0x00));
	printf("SPI %02x\n", SPI_TX(0xFF));
	printf("SPI %02x\n", SPI_TX(0xFF));

	PORTB|=1; // CS h

	_delay_ms(200);

	PORTB&=0xFE;

	// Check UUID
	SPI_TX(0x4B);
	SPI_TX(0xFF);
	SPI_TX(0xFF);
	SPI_TX(0xFF);
	SPI_TX(0xFF);
	printf("SPI ");
	for (int i=0; i<8; i++)
		printf("%02x",SPI_TX(0xFF));
	printf("\n");

	PORTB|=1; // CS h

	_delay_ms(20);

	// Write enable
	PORTB&=0xFE;
	SPI_TX(0x06);

	PORTB|=1;
	_delay_ms(20);
	PORTB&=0xFE;

	SPI_TX(0x02);
	//Addr 0x000000; /first page.
	SPI_TX(0x00);
	SPI_TX(0x00);
	SPI_TX(0x00);

	for (int i=0; i<=255; i++)
		SPI_TX(i);

	PORTB|=1;

	_delay_ms(20);
	PORTB&=0xFE;
	SPI_TX(0x06);

	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;

	SPI_TX(0x02);
	//Addr 0x000000; /last page.
	SPI_TX(0x03);
	SPI_TX(0xFF);
	SPI_TX(0x00);

	for (int i=0; i<=255; i++)
		SPI_TX(i);

	PORTB|=1;
	printf("WROTE FLASH\n");

	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0x06);

	PORTB|=1;
	_delay_ms(20);
	// clear flash
	PORTB&=0xFE;
	SPI_TX(0x60);

	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;

	SPI_TX(0x03);
	//Addr 0x000000; /first page.
	SPI_TX(0x00);
	SPI_TX(0x00);
	SPI_TX(0x00);

	for (unsigned long i=0; i<=0x3FFFF; i++)
	{
		if (0xFF!=SPI_TX(0xFF))
		{
			printf("NOT EMPTY AT %lu\n",i);
		}
	}
	printf("CLEARED\n");

	PORTB|=1;

	_delay_ms(100);

	PORTB&=0xFE;

	SPI_TX(0x03);
	//Addr 0x000000; /first page.
	SPI_TX(0x00);
	SPI_TX(0x00);
	SPI_TX(0x00);

	for (int i=0; i<=255; i++)
	{
		if (i!=SPI_TX(0xFF))
		{
			printf("MISMATCH AT %d\n",i);
		}
	}
	PORTB|=1;
	_delay_ms(20);
	PORTB&=0xFE;
	SPI_TX(0x06); // enable WR
	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0x05); // read SR
	printf("WEL: %02x\n",SPI_TX(0xFF)&0x02);
	PORTB|=1;

	PORTB&=0xFE;
	SPI_TX(0x04); // disable WR
	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0x05); // read SR.
	printf("WEL: %02x\n",SPI_TX(0xFF)&0x02);
	PORTB|=1;

	PORTB&=0xFE;
	// Call the erase commands without WEL to make sure they don't erase...
	SPI_TX(0xC7); // chip erase.
	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0x60); // chip erase.
	PORTB|=1;
	_delay_ms(20);
	PORTB&=0xFE;

	SPI_TX(0x20); // sector erase.
	SPI_TX(0x00); SPI_TX(0x00); SPI_TX(0x00);
	PORTB|=1;
	_delay_ms(20);
	PORTB&=0xFE;

	SPI_TX(0x52); // sector erase.
	SPI_TX(0x00); SPI_TX(0x00); SPI_TX(0x00);
	PORTB|=1;
	_delay_ms(20);
	PORTB&=0xFE;

	SPI_TX(0xD8); // sector erase.
	SPI_TX(0x00); SPI_TX(0x00); SPI_TX(0x00);
	PORTB|=1;
	_delay_ms(20);
	PORTB&=0xFE;

	SPI_TX(0x03);
	//Addr 0x000000; /first page.
	SPI_TX(0x03);
	SPI_TX(0xFF);
	SPI_TX(0x00);

	for (int i=0; i<=255; i++)
	{
		if (i!=SPI_TX(0xFF))
		{
			printf("MISMATCH AT 0x3FF+ %d\n",i);
		}
	}
	PORTB|=1;

	printf("FLASH DONE\n");

	DDRH = 0x00;
	loop_until_bit_is_clear(PINH,6);

	PORTB&=0xFE;
	SPI_TX(0x06); // WR EN
	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0x20); // sector erase.
	SPI_TX(0x00); SPI_TX(0x00); SPI_TX(0x00);
	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0x03);
	//Addr 0x000000; /first page.
	SPI_TX(0x00);
	SPI_TX(0x00);
	SPI_TX(0x00);

	for (unsigned long i=0; i<=0x3FFFF; i++)
	{
		if (0xFF!=SPI_TX(0xFF))
		{
			printf("NOT EMPTY AT %lu\n",i);
			break;
		}
	}
	PORTB|=1;

	PORTB&=0xFE;
	SPI_TX(0x06); // WR EN
	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0x52); // 32k erase.
	SPI_TX(0x00); SPI_TX(0x80); SPI_TX(0x00);
	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0x03);
	//Addr 0x000000; /2nd block
	SPI_TX(0x00);
	SPI_TX(0x80);
	SPI_TX(0x00);

	for (unsigned long i=0; i<=32769; i++)
	{
		if (0xFF!=SPI_TX(0xFF))
		{
			printf("NOT EMPTY AT %lu\n",i);
			break;
		}
	}
	PORTB|=1;

	PORTB&=0xFE;
	SPI_TX(0x06); // WR EN
	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0xD8); // 64k erase.
	SPI_TX(0x01); SPI_TX(0x00); SPI_TX(0x00);
	PORTB|=1;
	_delay_ms(20);

	PORTB&=0xFE;
	SPI_TX(0x03);
	//Addr 0x000000; /3rd block
	SPI_TX(0x01);
	SPI_TX(0x00);
	SPI_TX(0x00);

	for (unsigned long i=0; i<=65537; i++)
	{
		if (0xFF!=SPI_TX(0xFF))
		{
			printf("NOT EMPTY AT %lu\n",i);
			break;
		}
	}
	cli();


	printf("FINISHED\n");

	// this quits the simulator, since interupts are off
	// this is a "feature" that allows running tests cases and exit
	sleep_cpu();
}
