/*

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


#ifndef __VOLTAGE_H___
#define __VOLTAGE_H___

#include "sim_irq.h"

enum {
	IRQ_VOLT_ADC_TRIGGER_IN = 0,
	IRQ_VOLT_ADC_VALUE_OUT,
	IRQ_VOLT_VALUE_IN,		
	IRQ_VOLT_DIGITAL_OUT,	
	IRQ_VOLT_COUNT
};

typedef struct voltage_t {
	avr_irq_t *	irq;		// irq list
	struct avr_t *avr;		// keep it around so we can pause it
	uint8_t		adc_mux_number;
	float fvScale;
	float	fCurrent;
} voltage_t;

void
voltage_init(
		struct avr_t * avr,
		voltage_t *p,
		int adc_mux_number,
		float fvscale, // voltage scale factor to bring it in line with the ADC 0-5v input.
		float start_value );

void
voltage_set(
		voltage_t *p,
		float val);


#endif /* __VOLTAGE_H___ */
