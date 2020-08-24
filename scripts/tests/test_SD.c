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

static int uart_putchar(char c, FILE *stream)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                         _FDEV_SETUP_WRITE);


uint8_t buffer[20];
volatile uint8_t done = 0;
volatile uint8_t bLine = 0;

void USART_init(void)
{
    #define BAUD_PRESCALER (((F_CPU / (115200 * 16UL))) - 1)
    UBRR0 = BAUD_PRESCALER;         // Set the baud rate prescale rate register
	UCSR0C = ((0<<USBS0)|(1 << UCSZ01)|(1<<UCSZ00));   // Set frame format: 8data, 1 stop bit. See Table 22-7 for details
	UCSR0B = ((1<<RXEN0)|(1<<TXEN0)|(1 << RXCIE0));       // Enable receiver and transmitter
}

ISR(USART0_RX_vect)
{
	buffer[done] = UDR0;
	if (buffer[done]=='\n')
		bLine++;
	done++;
}

uint8_t SPI_TX(uint8_t cData){

   SPDR = cData;
   while(!(SPSR & (1<<SPIF)))
      ;
   return SPDR;
}

uint8_t SDTXB(uint8_t uiCmd)
{
	PORTL&=0x7f;
	uint8_t tmp = SPI_TX(uiCmd);
	PORTL|=0x80;
	return tmp;
}


void SDTX(uint8_t uiCmd, uint64_t uiData, unsigned int uiRespLen)
 {
	PORTL &= 0x7f;
	SPI_TX(uiCmd);
	for (unsigned int i=0; i<5; i++)
	{
		uint8_t out = (uiData >> (8U*(3U-i)) & 0xFF);
		SPI_TX(out);
	}
	printf("REPLY ");
	for (unsigned int i=0; i<uiRespLen; i++)
	{
		printf("%02x",SPI_TX(0xFF));
	}
	printf("\n");
	PORTL|=0x80;
 }

 void WriteBlock(unsigned long long uiAddr)
 {
	PORTL &= 0x7f;
	SPI_TX(24u);
	for (unsigned int i=0; i<5; i++)
	{
		uint8_t out = (uiAddr >> (8U*(3U-i)) & 0xFF);
		SPI_TX(out);
	}
	SPI_TX(0xFF);
	SPI_TX(0xFE);
	for (unsigned int i=0; i<256; i++)
	{
		SPI_TX(i &0xFFu);
	}
	for (unsigned int i=0; i<256; i++)
	{
		SPI_TX(i &0xFFu);
	}
	// fake crc
	SPI_TX(0xFF);
	SPI_TX(0xFF);
	SPI_TX(0xFF);
	PORTL|=0x80;
 }

int main()
{
	stdout = &mystdout;
	USART_init();
	// setup SPI
	DDRB = ((1<<DDB2)|(1<<DDB1)); //spi pins on port b MOSI SCK,SS outputs
  	SPCR = ((1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<CPOL)|(1<<CPHA));  // SPI enable, Master, f/16

	sei();
	DDRL = 0b10111111;


	if (PINL&(1u<<6))
		printf("NO CARD\n");
	else
		printf("CARD\n");

	printf("READY\n");

	if (PINL&(1u<<6))
		printf("NO CARD\n");
	else
		printf("CARD\n");

	printf("REPLY %02x\n",SDTXB(0xFF));


	SDTX(0x00,0xFFFFFFFFFFULL,1);

	// Illegal command
	SDTX(0x03,0xFFFFFFFFFFULL,1);

	SDTX(0x08,0xFFFFFFFFFFULL,5);

	SDTX(0x09,0xFFFFFFFFFFULL,16);

	SDTX(0x0C,0xFFFFFFFFFFULL,2);

	SDTX(0x0D,0xFFFFFFFFFFULL,2);

	SDTX(0x10,0x200ULL,1);

	// read OCR
	SDTX(58u,0xFFFFFFFFFFULL,5);

	SDTX(41u,0xFFFFFFFFFFULL,1);
	SDTX(55u,0xFFFFFFFFFFULL,1);

	// Read a block, address OOR
	SDTX(0x11,0xFFFFFFFFFFULL,1);

	// Read a block, address misalign
	SDTX(0x11,0x0ULL,516);

	// write a block, address OOR
	SDTX(24u,0xFFFFFFFFFFULL,1);

	WriteBlock(1);
	WriteBlock(131071ull);
	printf("WRITTEN\n");

	while(!(PINL&(1u<<6)));

	printf("CARD REMOVED\n");
	SDTX(0x00,0xFFFFFFFFFFULL,1);

	while((PINL&(1u<<6)));
	printf("CARD MOUNTED\n");

	SDTX(0x11,0x1ULL,516);
	SDTX(0x11,131071ULL,516);

	cli();

	printf("FINISHED\n");

	// this quits the simulator, since interupts are off
	// this is a "feature" that allows running tests cases and exit
	sleep_cpu();
}
