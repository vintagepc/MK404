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


#define TX_B PORTH &= 0xFE
#define TX_E PORTH |= 0x01

#define RHW0 0x41
#define WHW0 0x40

#define BANKB 0x10

#define DUMP(x) printf("%02x\n",x)
#define DUMPTX(x) DUMP(SPI_TX(0xFF))

enum {
	R_IODIR,
	R_IPOL,
	R_GPINTEN,
	R_DEFVAL,
	R_INTCON,
	R_IOCON,
	R_GPPU,
	R_INTF,
	R_INTCAP,
	R_GPIO,
	R_OLAT,
	R_END
};

void check_read(uint8_t val)
{
	uint8_t reply = SPI_TX(0xFF);
	if (val != reply)
	{
		printf("BAD RX: Expected %02X but got %02X\n", val, reply);
	}
}

int main()
{
	stdout = &mystdout;
	sei();

	DDRB = ((1<<DDB2)|(1<<DDB1)); //spi pins on port b MOSI SCK,SS outputs
  	SPCR = ((1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<CPOL)|(1<<CPHA));  // SPI enable, Master, f/16
	DDRH = 0b00000001; // CS control lines.

	printf("READY\n");

	uint8_t default_b0[0x15+1] = {0};
	default_b0[0] = 0xFF;
	default_b0[1] = 0xFF;

	// defaults for bank = 1
	uint8_t default_b1[0x0A+1] = {0};
	default_b1[0] = 0xFF;
	default_b1[0x05] = 0x80;

	// Use SEQOP and read all registers.
	printf("READ SEQOP\n");
	TX_B;
	SPI_TX(RHW0);
	SPI_TX(R_IODIR);
	for (int i=0; i<sizeof(default_b0); i++)
	{
		check_read(default_b0[i]);
	}
	TX_E;

	// disable address incrementing
	printf("SET SEQOP\n");
	TX_B;
	SPI_TX(WHW0);
	SPI_TX(0x0A); // IOCON in bank0 mode.
	SPI_TX(0b100000);
	TX_E;

	// read IODIR and check SEQOP is 1 (read IODIR 4 times).
	TX_B;
	SPI_TX(RHW0);
	SPI_TX(R_IODIR);
	check_read(0xFF);
	check_read(0xFF);
	check_read(0xFF);
	check_read(0xFF);
	TX_E;

	// Change to bank mode 1
	printf("SET BANK\n");
	TX_B;
	SPI_TX(WHW0);
	SPI_TX(0x0A); // IOCON in bank0 mode.
	SPI_TX(0b10000000);
	TX_E;

	// Use SEQOP and read all registers.
	printf("READ SEQOP B0\n");
	TX_B;
	SPI_TX(RHW0);
	SPI_TX(R_IODIR);
	for (int i=0; i<sizeof(default_b1); i++)
	{
		check_read(default_b1[i]);
	}
	TX_E;

	printf("READ SEQOP B1\n");
	TX_B;
	SPI_TX(RHW0);
	SPI_TX(R_IODIR | BANKB);
	for (int i=0; i<10; i++)
	{
		check_read(default_b1[i]);
	}
	TX_E;

	// write OLATA
	printf("SET OLATA\n");
	TX_B;
	SPI_TX(WHW0);
	SPI_TX(R_OLAT);
	SPI_TX(0b11110000);
	TX_E;

	// Check GPIOB to make sure it hasn't updated (all A pins in input mode).
	TX_B;
	SPI_TX(RHW0);
	SPI_TX(R_GPIO | BANKB);
	check_read(0x00);
	TX_E;

	// write OLATA
	printf("SET IODIR\n");
	TX_B;
	SPI_TX(WHW0);
	SPI_TX(R_IODIR);
	SPI_TX(0b01010000);
	TX_E;

	TX_B;
	SPI_TX(WHW0);
	SPI_TX(R_OLAT);
	SPI_TX(0b11110000);
	TX_E;

	// Check GPIOB to make sure only output pins have updated.
	TX_B;
	SPI_TX(RHW0);
	SPI_TX(R_GPIO | BANKB);
	check_read(0b00000101);
	TX_E;

	// reset pins
	printf("RESET\n");
	TX_B;
	SPI_TX(WHW0);
	SPI_TX(R_OLAT);
	SPI_TX(0b00000000);
	TX_E;

	// Check GPIOB
	TX_B;
	SPI_TX(RHW0);
	SPI_TX(R_GPIO | BANKB);
	check_read(0b00000000);
	TX_E;

	// Repeat for the B GPIOS
	// write OLATB
	printf("SET IODIRB\n");
	TX_B;
	SPI_TX(WHW0);
	SPI_TX(R_IODIR);
	SPI_TX(0xFF);
	TX_E;
	TX_B;
	SPI_TX(WHW0);
	SPI_TX(R_IODIR | BANKB);
	SPI_TX(0b10010000);
	TX_E;

	TX_B;
	SPI_TX(WHW0);
	SPI_TX(R_OLAT | BANKB);
	SPI_TX(0b11110000);
	TX_E;

	// Check GPIOB to make sure only output pins have updated.
	TX_B;
	SPI_TX(RHW0);
	SPI_TX(R_GPIO);
	check_read(0b00000110);
	TX_E;

	// reset pins
	printf("RESET\n");
	TX_B;
	SPI_TX(WHW0);
	SPI_TX(R_OLAT | BANKB);
	SPI_TX(0b00000000);
	TX_E;

	// Check GPIOB
	TX_B;
	SPI_TX(RHW0);
	SPI_TX(R_GPIO);
	check_read(0b00000000);
	TX_E;

	printf("FINISHED\n");

	cli();


	// this quits the simulator, since interupts are off
	// this is a "feature" that allows running tests cases and exit
	sleep_cpu();
}
