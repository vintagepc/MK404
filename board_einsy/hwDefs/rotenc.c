/*
 rotenc.c

 Copyright 2018 Doug Szumski <d.s.szumski@gmail.com>

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
#include <stdio.h>
#include <string.h>

#include "sim_avr.h"
#include "rotenc.h"

const rotenc_pins_t state_table[ROTENC_STATE_COUNT] = {
	{ 0, 0 },
	{ 1, 0 },
	{ 1, 1 },
	{ 0, 1 }
};

static avr_cycle_count_t
rotenc_state_change(
	avr_t * avr,
	avr_cycle_count_t when,
	void * param)
{
	rotenc_t * rotenc = (rotenc_t *) param;

	switch (rotenc->direction) {
		case ROTENC_CW_CLICK:
			// Advance phase forwards
			if (++rotenc->phase >= ROTENC_STATE_COUNT) {
				rotenc->phase = 0;
			}
			if (rotenc->verbose) {
				printf("ROTENC: CW twist, pins A:%x, B:%x\n",
					state_table[rotenc->phase].a_pin,
					state_table[rotenc->phase].b_pin);
			}
			break;
		case ROTENC_CCW_CLICK:
			// Advance phase backwards
			if (--rotenc->phase < 0) {
				rotenc->phase = ROTENC_STATE_COUNT - 1;
			}
			if (rotenc->verbose) {
				printf("ROTENC: CCW twist, pins: A:%x, B:%x\n",
					state_table[rotenc->phase].a_pin,
					state_table[rotenc->phase].b_pin);
			}
			break;
		default:
			// Invalid direction
			break;
	}
	avr_raise_irq(
		rotenc->irq + IRQ_ROTENC_OUT_A_PIN,
		state_table[rotenc->phase].a_pin);
	avr_raise_irq(
		rotenc->irq + IRQ_ROTENC_OUT_B_PIN,
		state_table[rotenc->phase].b_pin);

	if(rotenc->iPulseCt-- >0) // Continue ticking the encoder
	{
		avr_cycle_timer_register_usec(avr, ROTENC_PULSE_DURATION_US,rotenc_state_change,param);
	}
	return 0;
}

static avr_cycle_count_t
rotenc_button_auto_release(
	avr_t * avr,
	avr_cycle_count_t when,
	void * param)
{
	rotenc_t * rotenc = (rotenc_t *) param;
	avr_raise_irq(rotenc->irq + IRQ_ROTENC_OUT_BUTTON_PIN, 0);
	if (rotenc->verbose) {
		printf("ROTENC: Button release\n");
	}
	return 0;
}

void
_rotenc_button_press(rotenc_t * rotenc, uint32_t duration)
{
	// Press down
	if (rotenc->verbose) {
		printf("ROTENC: Button press\n");
	}
	avr_raise_irq(rotenc->irq + IRQ_ROTENC_OUT_BUTTON_PIN, 1);

	// Pull up later
	avr_cycle_timer_register_usec(
		rotenc->avr,
		duration,
		rotenc_button_auto_release,
		rotenc);
}

void rotenc_button_press(rotenc_t *rotenc)
{
	_rotenc_button_press(rotenc, ROTENC_BUTTON_DURATION_US);
}

void rotenc_button_press_hold(rotenc_t *rotenc)
{
	_rotenc_button_press(rotenc, ROTENC_BUTTON_DURATION_LONG_US);
}

/*
 * Simulates one "twist" pulse of the rotary encoder.
 */
void
rotenc_twist(
	rotenc_t * rotenc,
	rotenc_twist_t direction)
{
	rotenc->direction = direction;

	rotenc->iPulseCt = 6;

	avr_cycle_timer_register_usec(
		rotenc->avr,
		ROTENC_PULSE_DURATION_US,
		rotenc_state_change,
		rotenc);



}

static const char * _rotenc_irq_names[IRQ_ROTENC_COUNT] = {
	[IRQ_ROTENC_OUT_A_PIN] = ">rotenc_a_pin.out",
	[IRQ_ROTENC_OUT_B_PIN] = ">rotenc_b_pin.out",
	[IRQ_ROTENC_OUT_BUTTON_PIN] = ">rotenc_button_pin.out",
};

void
rotenc_init(
	avr_t *avr,
	rotenc_t * rotenc)
{
	memset(rotenc, 0, sizeof(*rotenc));
	rotenc->irq = avr_alloc_irq(
			&avr->irq_pool,
			0,
			IRQ_ROTENC_COUNT,
			_rotenc_irq_names);
	rotenc->avr = avr;
	//rotenc->verbose =1;
}

