/*



*/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "uart_logger.h"
#include "avr_uart.h"
#include <fcntl.h>


//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif


static void
uart_logger_in_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
	uart_logger_t * this = (uart_logger_t*)param;
    // Dump to file
    uint8_t c = value;
    write(this->fdOut,&c,1);
    // and terminal

    printf("UART%c: 0x%02x\n",this->chUart,c);
}

static const char * irq_names[IRQ_UART_LOGGER_COUNT] = {
	[IRQ_UART_LOGGER_BYTE_IN] = "8<uart_pty.in",
};

void
uart_logger_init(
		struct avr_t * avr,
		uart_logger_t * this)
{
	memset(this, 0, sizeof(*this));

	this->avr = avr;
	this->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_UART_LOGGER_COUNT, irq_names);
	avr_irq_register_notify(this->irq + IRQ_UART_LOGGER_BYTE_IN, uart_logger_in_hook, this);
    
}

void
uart_logger_stop(
		uart_logger_t * this)
{
	close(this->fdOut);
}

void
uart_logger_connect(
		uart_logger_t * this,
		char uart)
{
	// disable the stdio dump, as we're pritning in hex. 
	uint32_t f = 0;
	avr_ioctl(this->avr, AVR_IOCTL_UART_GET_FLAGS(uart), &f);
	f &= ~AVR_UART_FLAG_STDIO;
	avr_ioctl(this->avr, AVR_IOCTL_UART_SET_FLAGS(uart), &f);

	avr_irq_t * src = avr_io_getirq(this->avr, AVR_IOCTL_UART_GETIRQ(uart), UART_IRQ_OUTPUT);
	if (src) {
		avr_connect_irq(src, this->irq + IRQ_UART_LOGGER_BYTE_IN);
	}
    this->chUart = uart;
    snprintf(this->fileName,49,"UART%c_out.bin",uart);

    // open the file
	this->fdOut = open(this->fileName, O_RDWR|O_CREAT, 0644);
	if (this->fdOut < 0) {
		perror(this->fileName);
	}
	// Truncate the file (start new)
	(void)ftruncate(this->fdOut, 0);

    printf("UART %c is now logging to %s\n",this->chUart,this->fileName);

}
