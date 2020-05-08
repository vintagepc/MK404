/*
	ADC_Buttons.cpp - For button matrices with various voltage levels/inputs.

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "avr_adc.h"

#include "ADC_Buttons.h"

uint ADC_Buttons::OnADCRead(struct avr_irq_t * irq, uint32_t value)
{
    //if (raw < 50) return Btn::right;
	//if (raw > 80 && raw < 100) return Btn::middle;
	//if (raw > 160 && raw < 180) return Btn::left;

	uint32_t iVOut = 5000;
    if (m_uiCurBtn == 1)
        iVOut = 170;
    else if (m_uiCurBtn == 2)
        iVOut = 90;
    else if (m_uiCurBtn ==3)
        iVOut = 25;

    return iVOut;
}

void ADC_Buttons::Init(struct avr_t * avr, uint8_t adc_mux_number)
{
	_Init(avr, adc_mux_number, this);

}

void ADC_Buttons::Push(uint8_t uiBtn)
{
	m_uiCurBtn = uiBtn;
}
