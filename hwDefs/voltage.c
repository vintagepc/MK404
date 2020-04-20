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

#include "voltage.h"

// VOLT_DIV_REF * ((float)current_voltage_raw_pwr / (1023 * OVERSAMPLENR)) / VOLT_DIV_FAC;
/*
 * Called when the ADC could use a new value
 * The value returned is NOT in "ADC" mode, it's in millivolts
 */
static void voltage_adc_hook(
		struct avr_irq_t * irq, uint32_t value, void * param)
{
    voltage_t *this = (voltage_t*) param;
	union {
		avr_adc_mux_t v;
		uint32_t l;
	} u = { .l = value };
	avr_adc_mux_t v = u.v;

	if (v.src != this->adc_mux_number)
		return;

	uint32_t iVOut =  (this->fCurrent*this->fvScale)*1000*5;
    avr_raise_irq(this->irq + IRQ_VOLT_ADC_VALUE_OUT,iVOut);
	//printf("VOLT out: %u\n",iVOut);
    return;

}

static void voltage_in_hook(
		struct avr_irq_t * irq, uint32_t value, void * param)
{
	 voltage_t *this = (voltage_t*) param;
	float fv = ((float)value) / 256;
	this->fCurrent = fv;
}

static const char * irq_names[IRQ_VOLT_COUNT] = {
	[IRQ_VOLT_ADC_TRIGGER_IN] = "8<volt.trigger",
	//[IRQ_TERM_TEMP_VALUE_OUT] = "16>thermistor.out",
	[IRQ_VOLT_VALUE_IN] = "16<volt.in",
	[IRQ_VOLT_ADC_VALUE_OUT] = "16>volt.valout"
};

void
voltage_init(
		struct avr_t * avr,
		voltage_t *p,
		int adc_mux_number,
		float fvScale,
		float start )
{
	p->avr = avr;
	p->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_VOLT_COUNT, irq_names);
	avr_irq_register_notify(p->irq + IRQ_VOLT_ADC_TRIGGER_IN, voltage_adc_hook, p);
	avr_irq_register_notify(p->irq + IRQ_VOLT_VALUE_IN, voltage_in_hook, p);

	p->fvScale = fvScale;
	p->adc_mux_number = adc_mux_number;
	p->fCurrent = start;

	avr_irq_t * src = avr_io_getirq(p->avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER);
	avr_irq_t * dst = avr_io_getirq(p->avr, AVR_IOCTL_ADC_GETIRQ, adc_mux_number);
	if (src && dst) {
		avr_connect_irq(src, p->irq + IRQ_VOLT_ADC_TRIGGER_IN);
		avr_connect_irq(p->irq + IRQ_VOLT_ADC_VALUE_OUT, dst);
	}
	printf("%s on ADC %d start %.2f\n", __func__, adc_mux_number, p->fCurrent);
}

void
voltage_set(
		voltage_t *p,
		float val )
{
	uint32_t value = val * 256;

	avr_raise_irq(p->irq + IRQ_VOLT_VALUE_IN, value);
}
