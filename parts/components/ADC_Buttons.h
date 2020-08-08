/*
	ADC_Buttons.h - For button matrices with various voltage levels/inputs.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

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

#include <stdint.h>            // for uint32_t, uint8_t
#include <stdio.h>             // for printf
#include <atomic>              // for atomic_uint8_t, __atomic_base
#include <string>              // for string
#include <vector>              // for vector
#include "ADCPeripheral.h"     // for ADCPeripheral
#include "BasePeripheral.h"    // for MAKE_C_TIMER_CALLBACK
#include "IScriptable.h"       // for ArgType, ArgType::Int, IScriptable::Li...
#include "Scriptable.h"        // for Scriptable
#include "sim_avr.h"           // for avr_t
#include "sim_avr_types.h"     // for avr_cycle_count_t
#include "sim_cycle_timers.h"  // for avr_cycle_timer_t

class ADC_Buttons:public ADCPeripheral, public Scriptable
{
	public:
		#define IRQPAIRS _IRQ(ADC_TRIGGER_IN,"<adc.trigger") _IRQ(ADC_VALUE_OUT,">adc.out") _IRQ(DIGITAL_OUT, ">adc.digital_out")
		#include "IRQHelper.h"

		ADC_Buttons(std::string strName):Scriptable(strName)
		{
			RegisterAction("Press","Presses the specified button in the array",0,{ArgType::Int});
			RegisterMenu("Push Left",ActBtnLeft);
			RegisterMenu("Push Middle",ActBtnMiddle);
			RegisterMenu("Push Right",ActBtnRight);
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

		enum Actions
		{
			ActPress,
			ActBtnLeft, // These conveniently have the same int value as the button they represent.
			ActBtnMiddle,
			ActBtnRight
		};

};
