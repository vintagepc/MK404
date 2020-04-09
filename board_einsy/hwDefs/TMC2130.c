/*
	TMC2130.c

    Simulates a TMC2130 driver for virtualizing Marlin on simAVR.
 */

#include <stdlib.h>
#include <stdio.h>
#include "sim_avr.h"
#include "TMC2130.h"

#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif

/*
 * called when a SPI byte is sent
 */
static void tmc2130_spi_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	tmc2130_t* this = (tmc2130_t*)param;
    if (!this->flags.bits.selected)
        return;
	TRACE(printf("TMC2130 %c: byte received: %02x\n",this->axis,value));
}

// Called when CSEL changes.
static void tmc2130_csel_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    tmc2130_t* this = (tmc2130_t*)param;
	TRACE(printf("TMC2130 %c: CSEL changed to %02x\n",this->axis,value));
    this->flags.bits.selected = (value==0); // NOTE: active low!
}

static const char * irq_names[IRQ_TMC2130_COUNT] = {
		[IRQ_TMC2130_SPI_BYTE_IN] = "8<tmc2130.spi.in",
		[IRQ_TMC2130_SPI_BYTE_OUT] = "8>tmc2130.chain",
        [IRQ_TMC2130_SPI_COMMAND_IN] = "40<tmc2130.cmd",
        [IRQ_TMC2130_SPI_CSEL] = "tmc2130.csel"
};

void
tmc2130_init(
		struct avr_t * avr,
		tmc2130_t *p,
        char axis)
{
	p->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_TMC2130_COUNT, irq_names);
    p->axis = axis;
	avr_irq_register_notify(p->irq + IRQ_TMC2130_SPI_BYTE_IN, tmc2130_spi_in_hook, p);
    avr_irq_register_notify(p->irq + IRQ_TMC2130_SPI_CSEL, tmc2130_csel_in_hook, p);
}

