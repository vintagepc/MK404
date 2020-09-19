/*
	ADC_Buttons.cpp - For button matrices with various voltage levels/inputs.

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

#include "ADC_Buttons.h"
#include "BasePeripheral.h"
#include "IKeyClient.h"
#include "IScriptable.h"
#include <iostream>

ADC_Buttons::ADC_Buttons(const std::string &strName, uint32_t uiDelay):Scriptable(strName),IKeyClient(), m_uiDelay{uiDelay}
{
	m_fcnRelease = MAKE_C_TIMER_CALLBACK(ADC_Buttons,AutoRelease);
	RegisterAction("Press","Presses the specified button in the array",0,{ArgType::Int});
	RegisterActionAndMenu("Push Left","Press left button",ActBtnLeft);
	RegisterActionAndMenu("Push Middle","Press middle button",ActBtnMiddle);
	RegisterActionAndMenu("Push Right","Press right button",ActBtnRight);

	RegisterKeyHandler('2', "MMU Left button");
	RegisterKeyHandler('3', "MMU Middle button");
	RegisterKeyHandler('4', "MMU Right button");
};

void ADC_Buttons::OnKeyPress(const Key& key)
{
	switch (key)
	{
		case '2':
		case '3':
		case '4':
			Push(key - '1'); // button numbers are 1/2/3
			break;
	}
}

uint32_t ADC_Buttons::OnADCRead(struct avr_irq_t *, uint32_t)
{
    //if (raw < 50) return Btn::right;
	//if (raw > 80 && raw < 100) return Btn::middle;
	//if (raw > 160 && raw < 180) return Btn::left;

	uint32_t iVOut = 5000;
    if (m_uiCurBtn == 1)
	{
        iVOut = 170*5000/1023;
	}
    else if (m_uiCurBtn == 2)
	{
        iVOut = 90*5000/1023;
	}
    else if (m_uiCurBtn ==3)
	{
        iVOut = 25*5000/1023;
	}
    return iVOut;
}

Scriptable::LineStatus ADC_Buttons::ProcessAction(unsigned int uiAct, const std::vector<std::string> &vArgs)
{
	switch (uiAct)
	{
		case ActPress:
		{
			uint8_t uiBtn = stoi(vArgs.at(0));
			if (uiBtn>0 && uiBtn <4)
			{
				Push(uiBtn);
				return LineStatus::Finished;
			}
			else
			{
				return LineStatus::Error;
			}
		}
		case ActBtnLeft:
		case ActBtnMiddle:
		case ActBtnRight:
			Push(uiAct);
			return LineStatus::Finished;
		default:
			return Scriptable::LineStatus::Unhandled;
	}
}

void ADC_Buttons::Init(struct avr_t * avr, uint8_t adc_mux_number)
{
	_Init(avr, adc_mux_number, this);

}

void ADC_Buttons::Push(uint8_t uiBtn)
{
	std::cout << "Pressing button " << uiBtn << '\n';
	m_uiCurBtn = uiBtn;
	RegisterTimerUsec(m_fcnRelease,m_uiDelay, this);
}

avr_cycle_count_t ADC_Buttons::AutoRelease(avr_t *, avr_cycle_count_t)
{
	std::cout  << GetName() << " button release\n";
	m_uiCurBtn = 0;
	return 0;
};
