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

// This portion is straight from the swi2c section of the Prusa firmware.
#define SWI2C_TMO         2048
#define SWI2C_RMSK   0x01 //read mask (bit0 = 1)
#define SWI2C_WMSK   0x00 //write mask (bit0 = 0)
#define SWI2C_ASHF   0x01 //address shift (<< 1)
#define SWI2C_DMSK   0x7f //device address mask


void __delay(void)
{
	_delay_us(1.5);
}

void swi2c_init(void)
{
	DDRD |=2;
	DDRD|=1;
	PORTD|=2; // SDA H
	PORTD|=1; // SCL H
	uint8_t i; for (i = 0; i < 100; i++)
		__delay();
}

void swi2c_start(void)
{
	PORTD&=~(2u); // SDA L
	__delay();
	PORTD&=~(1U); // SCL L
	__delay();
}

void swi2c_stop(void)
{
	PORTD|=1; // SCL H
	__delay();
	PORTD|=2; // SDA H
	__delay();
}

void swi2c_ack(void)
{
	PORTD&=~(2u); // SDA L
	__delay();
	PORTD|=1; // SCL H
	__delay();
	PORTD&=~(1U); // SCL L
	__delay();
}

uint8_t swi2c_wait_ack()
{
	DDRD &= ~(2u); // SDA IN
	__delay();
//	PORTD|=2; // SDA H
	__delay();
	PORTD|=1; // SCL H
//	__delay();
	uint8_t ack = 0;
	uint16_t ackto = SWI2C_TMO;
	while (!(ack = ((PIND&2)?0:1)) && ackto--) __delay();
	PORTD&=~(1U); // SCL L
	__delay();
	DDRD |=2;
	__delay();
	PORTD&=~(2u); // SDA L
	__delay();
	return ack;
}

uint8_t swi2c_read(void)
{
	PORTD|=2; // SDA H
	__delay();
	DDRD &= ~(2u); // SDA IN
	uint8_t data = 0;
	int8_t bit; for (bit = 7; bit >= 0; bit--)
	{
		PORTD|=1; // SCL H
		__delay();
		data |= ((PIND&2)?1:0) << bit;
		PORTD&=~(1U); // SCL L
		__delay();
	}
	DDRD |=2;
	return data;
}

void swi2c_write(uint8_t data)
{
	int8_t bit; for (bit = 7; bit >= 0; bit--)
	{
		if (data & (1 << bit)) PORTD|=2; // SDA H
		else PORTD&=~(2u); // SDA L
		__delay();
		PORTD|=1; // SCL H
		__delay();
		PORTD&=~(1U); // SCL L
		__delay();
	}
}

uint8_t swi2c_check(uint8_t dev_addr)
{
	swi2c_start();
	swi2c_write((dev_addr & SWI2C_DMSK) << SWI2C_ASHF);
	if (!swi2c_wait_ack()) { swi2c_stop(); return 0; }
	swi2c_stop();
	return 1;
}

uint8_t swi2c_readByte_A8(uint8_t dev_addr, uint8_t addr, uint8_t* pbyte)
{
	swi2c_start();
	swi2c_write(SWI2C_WMSK | ((dev_addr & SWI2C_DMSK) << SWI2C_ASHF));
	if (!swi2c_wait_ack()) { swi2c_stop(); return 0; }
	swi2c_write(addr & 0xff);
	if (!swi2c_wait_ack()) return 0;
	swi2c_stop();
	swi2c_start();
	swi2c_write(SWI2C_RMSK | ((dev_addr & SWI2C_DMSK) << SWI2C_ASHF));
	if (!swi2c_wait_ack()) return 0;
	uint8_t byte = swi2c_read();
	swi2c_stop();
	if (pbyte) *pbyte = byte;
	return 1;
}

uint8_t swi2c_writeByte_A8(uint8_t dev_addr, uint8_t addr, uint8_t* pbyte)
{
	swi2c_start();
	swi2c_write(SWI2C_WMSK | ((dev_addr & SWI2C_DMSK) << SWI2C_ASHF));
	if (!swi2c_wait_ack()) { swi2c_stop(); return 0; }
	swi2c_write(addr & 0xff);
	if (!swi2c_wait_ack()) return 0;
	swi2c_write(*pbyte);
	if (!swi2c_wait_ack()) return 0;
	swi2c_stop();
	return 1;
}

// -- End blatant copypasta -- //

void ReadReg(uint8_t addr)
{
	uint8_t uiReply = 0;
	if (!swi2c_readByte_A8(0x75, addr, &uiReply))
		printf("READ ERR\n");
	else
		printf("RD %02x: %02x\n",addr,uiReply);

}

void WriteReg(uint8_t addr, uint8_t data)
{
	if (!swi2c_writeByte_A8(0x75, addr, &data))
		printf("READ ERR\n");
}

int main()
{
	stdout = &mystdout;

	sei();

	swi2c_init();

	printf("READY\n");

	ReadReg(0x00);

	ReadReg(0x01);

	ReadReg(0x02);

	ReadReg(0x04);

	WriteReg(0x0d,0u);
	WriteReg(0x0e,240u);

	ReadReg(0x0d);
	ReadReg(0x0e);
	ReadReg(0x14);
	ReadReg(0x17);

	// Script should toggle filament now.

	ReadReg(0x14);
	ReadReg(0x17);
	for (int i=0; i<7; i++)
	{
		ReadReg(0x04);
		ReadReg(0x12);
		ReadReg(0x02);
	}
	ReadReg(0x04);
	ReadReg(0x12);
	printf("SYNC\n");

	ReadReg(0x04);
	ReadReg(0x14);
	ReadReg(0x17);

	// Script toggles jam here.
	ReadReg(0x14);

	ReadReg(0x14);

	ReadReg(0x14);

	cli();

	printf("FINISHED\n");

	// this quits the simulator, since interupts are off
	// this is a "feature" that allows running tests cases and exit
	sleep_cpu();
}
