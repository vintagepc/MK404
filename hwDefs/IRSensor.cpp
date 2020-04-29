/*
	IRSensor.cpp

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
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "IRSensor.h"

// ADC read trigger. 
void IRSensor::OnADCRead(struct avr_irq_t * irq, uint32_t value)
{
	union {
		avr_adc_mux_t v;
		uint32_t l;
	} u = { .l = value };
	avr_adc_mux_t v = u.v;

    if (v.src != m_uiMuxNr)
		return;
		
    float fVal;
    if (m_eCurrent != IR_AUTO)
        fVal = m_fIRVals[m_eCurrent];
    else if (m_bExternal)
        fVal = m_fIRVals[IR_FILAMENT_PRESENT];
    else
        fVal = m_fIRVals[IR_NO_FILAMENT];
    
	uint32_t iVOut =  (fVal)*1000;

	SendToADC(iVOut);
}

IRSensor::IRSensor(uint8_t uiMux):VoltageSrc(uiMux)
{

}

void IRSensor::Toggle()
{
	if (m_eCurrent == IR_AUTO)
		printf("NOTE: Overriding IR Auto setting!\n");

	if (m_eCurrent == IR_NO_FILAMENT)
	{
		printf("IRSensor: Filament present!\n");
		m_eCurrent = IR_FILAMENT_PRESENT;
	}
	else
	{
		printf("IRSensor: No filament present!\n");
		m_eCurrent = IR_NO_FILAMENT;
	}
}

void IRSensor::Set(IRState val)
{
	m_eCurrent = val;
}

void IRSensor::Auto_Input(uint32_t val)
{
	m_bExternal = val>0;
}
