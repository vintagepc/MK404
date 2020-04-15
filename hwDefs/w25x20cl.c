/*
	hc595.c

	This defines a sample for a very simple "peripheral" 
	that can talk to an AVR core.
	It is in fact a bit more involved than strictly necessary,
	but is made to demonstrante a few useful features that are
	easy to use.
	
	Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

 	This file is part of simavr.

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include "sim_avr.h"
#include "w25x20cl.h"

#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif

/*
 * called when a SPI byte is sent
 */
static void w25x20cl_spi_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	w25x20cl_t* this = (w25x20cl_t*)param;
	if (!this->flags.bits.selected)
		return;
	TRACE(printf("w25x20cl_t: byte received: %02x\n",value));
}

// Called when CSEL changes.
static void w25x20cl_csel_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	w25x20cl_t* this = (w25x20cl_t*)param;
	TRACE(printf("w25x20cl_t: CSEL changed to %02x\n",value));
	this->flags.bits.selected = (value==0); // NOTE: active low!
}

static const char * irq_names[IRQ_W25X20CL_COUNT] = {
		[IRQ_W25X20CL_SPI_BYTE_IN] = "8<w25x20cl.in",
		[IRQ_W25X20CL_SPI_BYTE_OUT] = "8>w25x20cl.chain",
};

void
w25x20cl_init(
		struct avr_t * avr,
		w25x20cl_t *p)
{
	p->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_W25X20CL_COUNT, irq_names);
	avr_irq_register_notify(p->irq + IRQ_W25X20CL_SPI_BYTE_IN, w25x20cl_spi_in_hook, p);
	avr_irq_register_notify(p->irq + IRQ_W25X20CL_SPI_CSEL, w25x20cl_csel_in_hook, p);
}

