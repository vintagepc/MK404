/*
	UART logger
 */


#ifndef __UART_LOGGER_H___
#define __UART_LOGGER_H___

#include <sim_avr.h>
#include "sim_irq.h"

enum {
	IRQ_UART_LOGGER_BYTE_IN = 0,
	IRQ_UART_LOGGER_COUNT
};

typedef struct uart_logger_t {
    avr_t * avr;
    avr_irq_t *irq;
	char fileName[50];
    char chUart;
	int 		fdOut; // File handle.
} uart_logger_t;

void
uart_logger_init(
		struct avr_t * avr,
		uart_logger_t * p);
void
uart_logger_stop(uart_logger_t * p);

void
uart_logger_connect(
		uart_logger_t * p,
		char uart);

#endif /* __UART_LOGGER_H___ */
