/*
	Thermistor.h - thermistor simulator.

	Original Copyright 2008-2012 Michel Pollet <buserror@gmail.com>

	Rewritten for C++ in 2020 by VintagePC <https://github.com/vintagepc/>

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

#include "ADCPeripheral.h"  // for ADCPeripheral
#include "IScriptable.h"    // for IScriptable::LineStatus
#include "Scriptable.h"     // for Scriptable
#include "gsl-lite.hpp"
#include "sim_avr.h"        // for avr_t
#include "sim_irq.h"        // for avr_irq_t
#include <cstdint>         // for uint32_t, uint8_t
#include <string>           // for string
#include <utility>
#include <vector>           // for vector

class Thermistor: public ADCPeripheral, public Scriptable
{

	public:

		#define IRQPAIRS _IRQ(ADC_TRIGGER_IN,"<adc.trigger") _IRQ(ADC_VALUE_OUT,">adc.out") _IRQ(TEMP_OUT,">temp.out") _IRQ(TEMP_IN, "<temp.in") _IRQ(DIGITAL_OUT,">temp.digital_out")
		#include "IRQHelper.h"

		// Creates a new thermistor with given starting/ambient temperature.
		explicit Thermistor(float fStartTemp = 25);

		// Registers with SimAVR on the given mux,
		void Init(avr_t *avr, uint8_t adc_mux_number);

		// Sets the thermistor table. You can feed this a marlin table def.
		void SetTable(const gsl::span<const int16_t> table, int oversampling);

		// Set the temperature explicitly.
		void Set(float fTemp);
	protected:
		LineStatus ProcessAction(unsigned int iAction, const vector<string> &args) override;

	private:

		uint32_t OnADCRead(avr_irq_t *irq, uint32_t value) override;

		enum Actions
		{
			OpenCircuit,
			Shorted,
			Connected,
			ActSetTemp
		};

		void OnTempIn(avr_irq_t *irq, uint32_t value);

		std::vector<std::pair<int16_t, int16_t>> m_vTable;
		int 		m_iOversampling = 16;
		float	m_fCurrentTemp = 25;
		Actions m_eState = Connected;
};
