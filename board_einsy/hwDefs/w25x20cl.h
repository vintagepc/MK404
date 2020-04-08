/*
	An SPI flash emulator for the external language flash
 */

#ifndef __W25X20CL_H__
#define __W25X20CL_H__

#include "sim_irq.h"

/*
 * this one is quite fun, it simulated a 74HC595 shift register
 * driven by an SPI signal.
 * For the interest of the simulation, they can be chained, but 
 * for practicality sake the shift register is kept 32 bits
 * wide so it acts as 4 of them "daisy chained" already. 
 */
enum {
	IRQ_W25X20CL_SPI_BYTE_IN = 0,	// if hooked to a byte based SPI IRQ
	IRQ_W25X20CL_SPI_BYTE_OUT,		// to chain them !!
	IRQ_W25X20CL_COUNT
};

typedef struct w25x20cl_t {
	avr_irq_t *	irq;		// irq list
	uint8_t value;
	// TODO...
} w25x20cl_t;

void
w25x20cl_init(
		struct avr_t * avr,
		w25x20cl_t *p);

#endif
