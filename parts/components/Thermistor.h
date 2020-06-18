/*
	Thermistor.h - thermistor simulator.

	Original Copyright 2008-2012 Michel Pollet <buserror@gmail.com>

	Rewritten for C++ in 2020 by VintagePC <https://github.com/vintagepc/>

 	This file is part of MK3SIM.

	MK3SIM is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK3SIM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK3SIM.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include "ADCPeripheral.h"
#include "Scriptable.h"

class Thermistor: public ADCPeripheral, public Scriptable
{

	public:

		#define IRQPAIRS _IRQ(ADC_TRIGGER_IN,"<adc.trigger") _IRQ(ADC_VALUE_OUT,">adc.out") _IRQ(TEMP_OUT,">temp.out") _IRQ(TEMP_IN, "<temp.in") _IRQ(DIGITAL_OUT,">temp.digital_out")
		#include "IRQHelper.h"

		// Creates a new thermistor with given starting/ambient temperature.
		Thermistor(float fStartTemp = 25);

		// Registers with SimAVR on the given mux,
		void Init(avr_t *avr, uint8_t adc_mux_number);

		// Sets the thermistor table. You can feed this a marlin table def.
		void SetTable(short *pTable, unsigned int uiEntries, int oversampling);

		// Set the temperature explicitly.
		void Set(float fTemp);
	protected:
		LineStatus ProcessAction(unsigned int iAction, const vector<string> &args);

	private:

		uint32_t OnADCRead(avr_irq_t *irq, uint32_t value);

		enum Actions
		{
			OpenCircuit,
			Shorted,
			Connected
		};

		void OnTempIn(avr_irq_t *irq, uint32_t value);

		short * m_pTable = nullptr;
		unsigned int m_uiTableEntries = 0;
		int 		m_iOversampling = 16;
		float	m_fCurrentTemp = 25;
		Actions m_eState = Connected;
};
