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


#ifndef __IRSENSOR_H___
#define __IRSENSOR_H___

#include "sim_irq.h"
#include "stdbool.h"

enum {
	IRQ_IRSENSOR_ADC_TRIGGER_IN = 0,
	IRQ_IRSENSOR_ADC_VALUE_OUT,
	IRQ_IRSENSOR_DIGITAL_OUT,	
	IRQ_IRSENSOR_COUNT
};

typedef enum IRState {
    IR_SHORT,
    IR_FILAMENT_PRESENT,
    IR_UNKNOWN,
    IR_NO_FILAMENT,
    IR_NOT_CONNECTED,
    IR_AUTO // Special state that only respects the auto value.
}IRState;

typedef struct irsensor_t {
	avr_irq_t *	irq;		// irq list
	struct avr_t *avr;		// keep it around so we can pause it
	uint8_t		adc_mux_number;
	IRState	eCurrent;
    bool bExternal;
} irsensor_t;

void
irsensor_init(
		struct avr_t * avr,
		irsensor_t *p,
		int adc_mux_number
	 );

void
irsensor_set(
		irsensor_t *p,
		IRState val);

void
irsensor_auto_input(
		irsensor_t *p,
		uint32_t val);

#endif /* __VOLTAGE_H___ */
