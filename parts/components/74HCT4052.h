/*
	74HCT4052.cpp - Mux component sim.

	Copyright 2021 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK404.

	MK404 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK404 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK404.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include "ADCPeripheral.h"     // for ADCPeripheral
#include "sim_avr.h"           // for avr_t
#include <cstdint>            // for uint32_t, uint8_t
#include <string>              // for string

class L74HCT4052:public ADCPeripheral
{
	public:
		// NOTE: While this is a combined chip comprising two
		// muxes, it makes sense to split due to the ADC architecture
		// and have each handled on its own, isolated to its single ADC line.
		#define IRQPAIRS \
			_IRQ(ADC_TRIGGER_IN,"<adc.trigger") \
			_IRQ(ADC_VALUE_OUT,">adc.out") \
			_IRQ(DIGITAL_OUT, ">adc.digital_out") \
			_IRQ(IN_0, "<mux.in_0") \
			_IRQ(IN_1, "<mux.in_1") \
			_IRQ(IN_2, "<mux.in_2") \
			_IRQ(IN_3, "<mux.in_3") \
			_IRQ(OUT_0, "<mux.in_0") \
			_IRQ(OUT_1, "<mux.in_1") \
			_IRQ(OUT_2, "<mux.in_2") \
			_IRQ(OUT_3, "<mux.in_3") \
			_IRQ(A_IN,"<mux.a") \
			_IRQ(B_IN,"<mux.b")
		#include "IRQHelper.h"

		L74HCT4052() = default;

		~L74HCT4052() = default;


		// someday... extend this with flexibility for any number of buttons/voltage levels.
		void Init(avr_t *avr, uint8_t uiMux);

	private:

		uint8_t m_select = 0;

		uint32_t OnADCRead(struct avr_irq_t * irq, uint32_t value) override;

};
