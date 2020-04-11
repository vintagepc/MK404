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

#include "thermistor.h"

/*
 * Called when the ADC could use a new value
 * The value returned is NOT in "ADC" mode, it's in millivolts
 */
static void thermistor_in_hook(
		struct avr_irq_t * irq, uint32_t value, void * param)
{
	thermistor_p p = (thermistor_p)param;
	union {
		avr_adc_mux_t v;
		uint32_t l;
	} u = { .l = value };
	avr_adc_mux_t v = u.v;

	if (v.src != p->adc_mux_number)
		return;

	short *t = p->table, *lt = NULL;
	for (int ei = 0; ei < p->table_entries; ei++, lt = t, t += 2) {
		if (t[1] <= p->current) {
			short tt = t[0];
			/* small linear regression between table samples */
			if (ei > 0 && t[1] < p->current) {
				short d_adc = t[0] - lt[0];
				float d_temp = t[1] - lt[1];
				float delta = p->current - t[1];
				tt = t[0] + (d_adc * (delta / d_temp));
			}
			// if (p->adc_mux_number==-1)
			// 	printf("simAVR ADC out value: %u\n",((tt / p->oversampling) * 5000) / 0x3ff);
			avr_raise_irq(p->irq + IRQ_TERM_ADC_VALUE_OUT,
						 ((tt / p->oversampling) * 5000) / 0x3ff);
			return;
		}
	}
	printf("%s(%d) temperature out of range (%.2f), we're screwed\n",
			__func__, p->adc_mux_number, p->current);
}

static void thermistor_value_in_hook(
		struct avr_irq_t * irq, uint32_t value, void * param)
{
	thermistor_p p = (thermistor_p)param;
	float fv = ((float)value) / 256;
	p->current = fv;

	avr_raise_irq(p->irq + IRQ_TERM_TEMP_VALUE_OUT, value);
}

static const char * irq_names[IRQ_TERM_COUNT] = {
	[IRQ_TERM_ADC_TRIGGER_IN] = "8<thermistor.trigger",
	[IRQ_TERM_TEMP_VALUE_OUT] = "16>thermistor.out",
	[IRQ_TERM_TEMP_VALUE_IN] = "16<thermistor.in",
};

void
thermistor_init(
		struct avr_t * avr,
		thermistor_p p,
		int adc_mux_number,
		short * table,
		int	table_entries,
		int oversampling,
		float start_temp )
{
	p->avr = avr;
	p->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_TERM_COUNT, irq_names);
	avr_irq_register_notify(p->irq + IRQ_TERM_ADC_TRIGGER_IN, thermistor_in_hook, p);
	avr_irq_register_notify(p->irq + IRQ_TERM_TEMP_VALUE_IN, thermistor_value_in_hook, p);

	p->oversampling = oversampling;
	p->table = table;
	p->table_entries = table_entries;
	p->adc_mux_number = adc_mux_number;
	p->current = start_temp;

	avr_irq_t * src = avr_io_getirq(p->avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER);
	avr_irq_t * dst = avr_io_getirq(p->avr, AVR_IOCTL_ADC_GETIRQ, adc_mux_number);
	if (src && dst) {
		avr_connect_irq(src, p->irq + IRQ_TERM_ADC_TRIGGER_IN);
		avr_connect_irq(p->irq + IRQ_TERM_ADC_VALUE_OUT, dst);
	}
	printf("%s on ADC %d start %.2f\n", __func__, adc_mux_number, p->current);
}

void
thermistor_set_temp(
		thermistor_p p,
		float temp )
{
	uint32_t value = temp * 256;
	p->current = temp;

	avr_raise_irq(p->irq + IRQ_TERM_TEMP_VALUE_OUT, value);
}
