/*
	thermistor.c

	Copyright 2008-2012 Michel Pollet <buserror@gmail.com>

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
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "IRSensor.h"

void IRSensor::OnADCRead(struct avr_irq_t * irq, uint32_t value)
{
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
