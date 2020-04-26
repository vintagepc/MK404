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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "avr_adc.h"

#include "mmu_buttons.h"

static void mmu_buttons_adc_hook(
		struct avr_irq_t * irq, uint32_t value, void * param)
{
    buttons_t *this = (buttons_t*) param;
	union {
		avr_adc_mux_t v;
		uint32_t l;
	} u = { .l = value };
	avr_adc_mux_t v = u.v;

	if (v.src != this->adc_mux_number)
		return;

    //if (raw < 50) return Btn::right;
	//if (raw > 80 && raw < 100) return Btn::middle;
	//if (raw > 160 && raw < 180) return Btn::left;

	uint32_t iVOut = 5000;
    if (this->curBtn == 1)
        iVOut = 170;
    else if (this->curBtn == 2)
        iVOut = 90;
    else if (this->curBtn ==3)
        iVOut = 25;

    avr_raise_irq(this->irq + IRQ_MMU_BUTTONS_ADC_VALUE_OUT,iVOut);
    return;

}

static const char * irq_names[IRQ_MMU_BUTTONS_COUNT] = {
	[IRQ_MMU_BUTTONS_ADC_TRIGGER_IN] = "8<buttons.trigger",
	[IRQ_MMU_BUTTONS_ADC_VALUE_OUT] = "8>buttons.valout"
};

void
mmu_buttons_init(
		struct avr_t * avr,
		buttons_t *p,
		int adc_mux_number)
{
	p->avr = avr;
	p->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_MMU_BUTTONS_COUNT, irq_names);
	avr_irq_register_notify(p->irq + IRQ_MMU_BUTTONS_ADC_TRIGGER_IN, mmu_buttons_adc_hook, p);


	p->curBtn = 0;
	p->adc_mux_number = adc_mux_number;
	
	avr_irq_t * src = avr_io_getirq(p->avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER);
	avr_irq_t * dst = avr_io_getirq(p->avr, AVR_IOCTL_ADC_GETIRQ, adc_mux_number);
	if (src && dst) {
		avr_connect_irq(src, p->irq + IRQ_MMU_BUTTONS_ADC_TRIGGER_IN);
		avr_connect_irq(p->irq + IRQ_MMU_BUTTONS_ADC_VALUE_OUT, dst);
    }
}

void
mmu_button_push(
		buttons_t *p,
		uint8_t val )
{
	p->curBtn = val;
}
