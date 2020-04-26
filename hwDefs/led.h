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


#ifndef __LED_H___
#define __LED_H___

#include "sim_irq.h"
#include "stdbool.h"
#include "sim_avr.h"

enum {
	IRQ_LED_IN = 0,
	IRQ_LED_COUNT
};

typedef struct led_t {
	avr_irq_t *	irq;		// irq list
	float fColor[3];
	bool bOn;
} led_t;

void
led_init(avr_t * avr,
		led_t *p,
	    uint32_t uiHexColor);

void drawLED_gl(led_t *this);
#endif /* __LED_H___ */
