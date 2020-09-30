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
#define TWI_FREQ 400000L

#define 	TW_START   0x08
#define 	TW_REP_START   0x10
#define 	TW_MT_SLA_ACK   0x18
#define 	TW_MT_SLA_NACK   0x20
#define 	TW_MT_DATA_ACK   0x28
#define 	TW_MT_DATA_NACK   0x30
#define 	TW_MT_ARB_LOST   0x38
#define 	TW_MR_ARB_LOST   0x38
#define 	TW_MR_SLA_ACK   0x40
#define 	TW_MR_SLA_NACK   0x48
#define 	TW_MR_DATA_ACK   0x50
#define 	TW_MR_DATA_NACK   0x58
#define 	TW_ST_SLA_ACK   0xA8
#define 	TW_ST_ARB_LOST_SLA_ACK   0xB0
#define 	TW_ST_DATA_ACK   0xB8
#define 	TW_ST_DATA_NACK   0xC0
#define 	TW_ST_LAST_DATA   0xC8
#define 	TW_SR_SLA_ACK   0x60
#define 	TW_SR_ARB_LOST_SLA_ACK   0x68
#define 	TW_SR_GCALL_ACK   0x70
#define 	TW_SR_ARB_LOST_GCALL_ACK   0x78
#define 	TW_SR_DATA_ACK   0x80
#define 	TW_SR_DATA_NACK   0x88
#define 	TW_SR_GCALL_DATA_ACK   0x90
#define 	TW_SR_GCALL_DATA_NACK   0x98
#define 	TW_SR_STOP   0xA0
#define 	TW_NO_INFO   0xF8
#define 	TW_BUS_ERROR   0x00
#define 	TW_STATUS_MASK (_BV(TWS7)|_BV(TWS6)|_BV(TWS5)|_BV(TWS4)|\
				_BV(TWS3))
#define 	TW_READ   1
#define 	TW_WRITE   0

#define 	TW_STATUS   (TWSR & TW_STATUS_MASK)
// This portion is straight from the twi section of the Prusa firmware.
void twi_init(void)
{
  // activate internal pullups for twi.
  //digitalWrite(SDA, 1);
  //digitalWrite(SCL, 1);

  // initialize twi prescaler and bit rate
  TWSR &= ~(_BV(TWPS0) | _BV(TWPS1));
  TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;

  /* twi bit rate formula from atmega128 manual pg 204
  SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
  note: TWBR should be 10 or higher for master mode
  It is 72 for a 16mhz Wiring board with 100kHz TWI */
}

void twi_disable(void)
{
  // deactivate internal pullups for twi.
  //digitalWrite(SDA, 0);
  //digitalWrite(SCL, 0);
}

static void twi_stop()
{
  TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWSTO);
}


static uint8_t twi_wait(uint8_t status)
{
  while(!(TWCR & _BV(TWINT)));
  if(TW_STATUS != status)
  {
      twi_stop();
      return 1;
  }
  return 0;
}


static uint8_t twi_start(uint8_t address, uint8_t reg)
{
  // send start condition
  TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWSTA);
  if(twi_wait(TW_START))
      return 1;

  // send address
  TWDR = TW_WRITE | (address << 1);
  TWCR = _BV(TWEN) | _BV(TWINT);
  if(twi_wait(TW_MT_SLA_ACK))
      return 2;

  // send register
  TWDR = reg;
  TWCR = _BV(TWEN) | _BV(TWINT);
  if(twi_wait(TW_MT_DATA_ACK))
      return 3;

  return 0;
}

uint8_t twi_r8(uint8_t address, uint8_t reg, uint8_t* data)
{
  if(twi_start(address, reg))
      return 1;

  // repeat start
  TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWSTA);
  if(twi_wait(TW_REP_START))
      return 2;

  // start receiving
  TWDR = TW_READ | (address << 1);
  TWCR = _BV(TWEN) | _BV(TWINT);
  if(twi_wait(TW_MR_SLA_ACK))
      return 3;

  // receive data
  TWCR = _BV(TWEN) | _BV(TWINT);
  if(twi_wait(TW_MR_DATA_NACK))
      return 4;

  *data = TWDR;

  // send stop
  twi_stop();
  return 0;
}


uint8_t twi_w8(uint8_t address, uint8_t reg, uint8_t data)
{
  if(twi_start(address, reg))
      return 1;

  // send data
  TWDR = data;
  TWCR = _BV(TWEN) | _BV(TWINT);
  if(twi_wait(TW_MT_DATA_ACK))
      return 2;

  // send stop
  twi_stop();
  return 0;
}

// -- End blatant copypasta -- //

void ReadReg(uint8_t addr)
{
	uint8_t uiReply = 0;
	if (twi_r8(0x75, addr, &uiReply))
		printf("READ ERR\n");
	else
		printf("RD %02x: %02x\n",addr,uiReply);

}

void WriteReg(uint8_t addr, uint8_t data)
{
	if (twi_w8(0x75, addr, data))
		printf("WRITE ERR\n");
}

int main()
{
	stdout = &mystdout;

	sei();

	twi_init();

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

	uint8_t bytein;
	// Make sure we don't get a reply to a different address.
	if (twi_r8(0x61, 0x00, &bytein))
	{
		printf("READ ERR\n");
	}
	else
	{
		printf("READ: %02x\n", bytein);
	}


	cli();

	while(1);

	printf("FINISHED\n");

	// this quits the simulator, since interupts are off
	// this is a "feature" that allows running tests cases and exit
	sleep_cpu();
}
