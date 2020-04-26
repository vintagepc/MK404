/*
	thermistor.c

	Copyright 2008-2012 Michel Pollet <buserror@gmail.com>

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
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "avr_adc.h"

#include "IRSensor.h"

// VOLT_DIV_REF * ((float)current_voltage_raw_pwr / (1023 * OVERSAMPLENR)) / VOLT_DIV_FAC;
/*
 * Called when the ADC could use a new value
 * The value returned is NOT in "ADC" mode, it's in millivolts
 */
static const float fIRVals[IR_AUTO] = 
{
    [IR_SHORT] = 0.1f,
    [IR_FILAMENT_PRESENT] = 0.4f,
    [IR_UNKNOWN] = 3.0f,
    [IR_NO_FILAMENT] = 4.5f,
    [IR_NOT_CONNECTED] = 4.9f
};


static void irsensor_adc_hook(
		struct avr_irq_t * irq, uint32_t value, void * param)
{
    irsensor_t *this = (irsensor_t*) param;
	union {
		avr_adc_mux_t v;
		uint32_t l;
	} u = { .l = value };
	avr_adc_mux_t v = u.v;

	if (v.src != this->adc_mux_number)
		return;
    float fVal;
    if (this->eCurrent != IR_AUTO)
        fVal = fIRVals[this->eCurrent];
    else if (this->bExternal)
        fVal = fIRVals[IR_FILAMENT_PRESENT];
    else
        fVal = fIRVals[IR_NO_FILAMENT];
    
	uint32_t iVOut =  (fVal)*1000;
    avr_raise_irq(this->irq + IRQ_IRSENSOR_ADC_VALUE_OUT,iVOut);
	if (iVOut>2200) // 2.2V, logic H
		avr_raise_irq(this->irq + IRQ_IRSENSOR_DIGITAL_OUT,1);
	else if (iVOut < 800) // 0.8v. L
		avr_raise_irq(this->irq + IRQ_IRSENSOR_DIGITAL_OUT,0);
	else
		avr_raise_irq_float(this->irq + IRQ_IRSENSOR_DIGITAL_OUT,0,1); 
    return;

}

static const char * irq_names[IRQ_IRSENSOR_COUNT] = {
	[IRQ_IRSENSOR_ADC_TRIGGER_IN] = "8<ir.trigger",
	[IRQ_IRSENSOR_DIGITAL_OUT] = ">ir.digitalOut",
	[IRQ_IRSENSOR_ADC_VALUE_OUT] = "16>ir.valout"
};

void
irsensor_init(
		struct avr_t * avr,
		irsensor_t *p,
		int adc_mux_number
	 )
{
	p->avr = avr;
	p->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_IRSENSOR_COUNT, irq_names);
	avr_irq_register_notify(p->irq + IRQ_IRSENSOR_ADC_TRIGGER_IN, irsensor_adc_hook, p);

	p->adc_mux_number = adc_mux_number;
	p->eCurrent = IR_NO_FILAMENT;

	avr_irq_t * src = avr_io_getirq(p->avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER);
	avr_irq_t * dst = avr_io_getirq(p->avr, AVR_IOCTL_ADC_GETIRQ, adc_mux_number);
	if (src && dst) {
		avr_connect_irq(src, p->irq + IRQ_IRSENSOR_ADC_TRIGGER_IN);
		avr_connect_irq(p->irq + IRQ_IRSENSOR_ADC_VALUE_OUT, dst);
	}
}

void
irsensor_set(
		irsensor_t *this,
		IRState val )
{
	this->eCurrent = val;
}

void
irsensor_auto_input(
		irsensor_t *this,
		uint32_t val )
{
	this->bExternal = val>0;
}
