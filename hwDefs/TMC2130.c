/*
	TMC2130.c

    Simulates a TMC2130 driver for virtualizing Marlin on simAVR.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "avr_ioport.h"
#include "sim_avr.h"
#include "avr_spi.h"
#include "avr_adc.h"
#include "TMC2130.h"

//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif


static void tmc2130_create_reply(tmc2130_t *this)
{
    if (!this->cmdOut.bitsIn.RW) // Last in was a read.
    {
        this->cmdOut.bitsOut.data = this->regs.raw[this->cmdOut.bitsIn.address];
    }
    // If the last was a write, the old data is left intact.

    // Set the status bits on the reply:
    this->cmdOut.bitsOut.driver_error = this->regs.defs.GSTAT.drv_err;
    this->cmdOut.bitsOut.reset_flag = this->regs.defs.GSTAT.reset;
    this->cmdOut.bitsOut.sg2 = this->regs.defs.DRV_STATUS.stallGuard;
    this->cmdOut.bitsOut.standstill = this->regs.defs.DRV_STATUS.stst;
    TRACE(printf("Reply built: %10lx\n",this->cmdOut.all));
}

// Called when a full command is ready to process. 
static void tmc2130_process_command(tmc2130_t *this)
{
    TRACE(printf("tmc2130 %c cmd: w: %x a: %02x  d: %08x\n",this->axis, this->cmdIn.bitsIn.RW, this->cmdIn.bitsIn.address, this->cmdIn.bitsIn.data));
    if (this->cmdIn.bitsIn.RW)
        this->regs.raw[this->cmdIn.bitsIn.address] = this->cmdIn.bitsIn.data;
    else
    {
        TRACE(printf("Read command on register: %02x\n", this->cmdIn.bitsIn.address));
    }
    tmc2130_create_reply(this);
    this->cmdOut = this->cmdIn;
}

/*
 * called when a SPI byte is received
 */
static void tmc2130_spi_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    tmc2130_t* this = (tmc2130_t*)param;
    if (!this->flags.bits.selected)
        return;
    // Clock out a reply byte:
    uint8_t byte = this->cmdOut.bytes[0];
    this->cmdOut.all<<=8;
    avr_raise_irq(this->irq + IRQ_TMC2130_SPI_BYTE_OUT,byte);
    TRACE(printf("TMC2130 %c: Clocking out %02x\n",this->axis,byte));

	TRACE(printf("TMC2130 %c: byte received: %02x\n",this->axis,value));
    this->cmdIn.all<<=8; // Shift bits up
    this->cmdIn.bytes[4] = value;
    


}

// Called when CSEL changes.
static void tmc2130_csel_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    tmc2130_t* this = (tmc2130_t*)param;
	//TRACE(printf("TMC2130 %c: CSEL changed to %02x\n",this->axis,value));
    this->flags.bits.selected = (value==0); // NOTE: active low!
    if (value == 1) // Just finished a CSEL
        tmc2130_process_command(this);
}

// Called when DIR pin changes.
static void tmc2130_dir_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    tmc2130_t* this = (tmc2130_t*)param;
	TRACE(printf("TMC2130 %c: DIR changed to %02x\n",this->axis,value));
    this->flags.bits.dir = value;
}

// Called when STEP is triggered.
static void tmc2130_step_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    tmc2130_t* this = (tmc2130_t*)param;
	TRACE(printf("TMC2130 %c: STEP changed to %02x\n",this->axis,value));
    if (this->flags.bits.dir)
        TRACE(printf("TMC2130 %c: Step++\n",this->axis));
    else
        TRACE(printf("TMC2130 %c: Step--\n",this->axis));
}

// Called when DRV_EN is triggered.
static void tmc2130_enable_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    tmc2130_t* this = (tmc2130_t*)param;
	TRACE(printf("TMC2130 %c: EN changed to %02x\n",this->axis,value));
    this->flags.bits.enable = value==0; // active low, i.e motors off when high.
}

static const char * irq_names[IRQ_TMC2130_COUNT] = {
		[IRQ_TMC2130_SPI_BYTE_IN] = "8<tmc2130.spi.in",
		[IRQ_TMC2130_SPI_BYTE_OUT] = "8>tmc2130.chain",
        [IRQ_TMC2130_SPI_COMMAND_IN] = "40<tmc2130.cmd",
        [IRQ_TMC2130_SPI_CSEL] = "tmc2130.csel",
        [IRQ_TMC2130_STEP_IN] = "tmc2130.step",
        [IRQ_TMC2130_DIR_IN] = "tmc2130.dir",
        [IRQ_TMC2130_ENABLE_IN] = "tmc2130.enable",
        [IRQ_TMC2130_DIAG_OUT] = "tmc2130.diag",
        [IRQ_TMC2130_DIAG_TRIGGER_IN] = "tmc2130.diagReq"
};

void
tmc2130_init(
		struct avr_t * avr,
		tmc2130_t *p,
        char axis, uint8_t iDiagPort) 
{
	p->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_TMC2130_COUNT, irq_names);
    p->axis = axis;
    memset(&p->cmdIn, 0, sizeof(p->cmdIn));
    p->byteIndex = 4;

    // Just wire right up to the AVR SPI
    avr_connect_irq(avr_io_getirq(avr,AVR_IOCTL_SPI_GETIRQ(0),SPI_IRQ_OUTPUT),
		p->irq + IRQ_TMC2130_SPI_BYTE_IN);
    avr_connect_irq(p->irq + IRQ_TMC2130_SPI_BYTE_OUT,
        avr_io_getirq(avr,AVR_IOCTL_SPI_GETIRQ(0),SPI_IRQ_INPUT));
    
	// avr_irq_t * src = avr_io_getirq(avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER);
	// avr_irq_t * dst = avr_io_getirq(avr, AVR_IOCTL_ADC_GETIRQ, iDiagADC);
	// if (src && dst) {
	// 	avr_connect_irq(src, p->irq + IRQ_TMC2130_DIAG_TRIGGER_IN);
	// 	avr_connect_irq(p->irq + IRQ_TMC2130_DIAG_OUT, dst);
    // }
	avr_irq_register_notify(p->irq + IRQ_TMC2130_SPI_BYTE_IN, tmc2130_spi_in_hook, p);
    avr_irq_register_notify(p->irq + IRQ_TMC2130_SPI_CSEL, tmc2130_csel_in_hook, p);
    avr_irq_register_notify(p->irq + IRQ_TMC2130_DIR_IN, tmc2130_dir_in_hook, p);
    avr_irq_register_notify(p->irq + IRQ_TMC2130_STEP_IN, tmc2130_step_in_hook, p);
    avr_irq_register_notify(p->irq + IRQ_TMC2130_ENABLE_IN, tmc2130_enable_in_hook, p);
    avr_connect_irq(p->irq + IRQ_TMC2130_DIAG_OUT,
		avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('K'),iDiagPort));	

    avr_raise_irq(p->irq + IRQ_TMC2130_DIAG_OUT,0);

}

