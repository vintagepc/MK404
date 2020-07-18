/*
	ADC_Buttons.h - For button matrices with various voltage levels/inputs.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

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
#include <stdint.h>         // for uint8_t, uint32_t
#include "sim_avr.h"        // for avr_t
#include "IScriptable.h"     // for ArgType, ArgType::Int, IScriptable::Line...
#include "Scriptable.h"      // for Scriptable
#include <string>
#include <atomic>

class ADC_Buttons:public ADCPeripheral, public Scriptable
{
	public:
		#define IRQPAIRS _IRQ(ADC_TRIGGER_IN,"<adc.trigger") _IRQ(ADC_VALUE_OUT,">adc.out") _IRQ(DIGITAL_OUT, ">adc.digital_out")
		#include "IRQHelper.h"

		ADC_Buttons(std::string strName):Scriptable(strName)
		{
			RegisterAction("Press","Presses the specified button in the array",0,{ArgType::Int});
		};


		// TODO.. extend this with flexibility for any number of buttons/voltage levels.
		void Init(avr_t *avr, uint8_t uiMux);

		// Pushes a given button: 1= left, 2 = middle, 3= right, 0 = none.
		void Push(uint8_t uiBtn);

	protected:
			LineStatus ProcessAction(unsigned int uiAct, const vector<string> &vArgs) override;
	private:

		inline avr_cycle_count_t AutoRelease(avr_t *avr, avr_cycle_count_t uiWhen)
		{
			printf("%s button release\n", GetName().c_str());
			m_uiCurBtn = 0;
			return 0;
		};

		avr_cycle_timer_t m_fcnRelease = MAKE_C_TIMER_CALLBACK(ADC_Buttons,AutoRelease);

		uint32_t OnADCRead(struct avr_irq_t * irq, uint32_t value) override;

		std::atomic_uint8_t m_uiCurBtn = {0};

};
