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


#ifndef __MMU_BUTTONS_H___
#define __MMU_BUTTONS_H___

#include "sim_irq.h"

enum {
	IRQ_MMU_BUTTONS_ADC_TRIGGER_IN = 0,
	IRQ_MMU_BUTTONS_ADC_VALUE_OUT,
	IRQ_MMU_BUTTONS_COUNT
};

typedef struct buttons_t {
	avr_irq_t *	irq;		// irq list
	struct avr_t *avr;		// keep it around so we can pause it
	uint8_t		adc_mux_number;
	uint8_t curBtn;
} buttons_t;

void
mmu_buttons_init(
		struct avr_t * avr,
		buttons_t *p,
		int adc_mux_number);

// 1= left, 2 = middle, 3= right, 0 = none.
void
mmu_button_push(
		buttons_t *p,
		uint8_t uiBtn);


#endif /* __MMU_BUTTONS_H___ */
