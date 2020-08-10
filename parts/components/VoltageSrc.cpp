/*
	VoltageSrc.cpp

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

#include "VoltageSrc.h"
#include "BasePeripheral.h"  // for MAKE_C_CALLBACK
#include "TelemetryHost.h"

uint32_t VoltageSrc::OnADCRead(struct avr_irq_t*, uint32_t)
{
    uint32_t iVOut =  (m_fCurrentV*m_fVScale)*1000*5;
	return iVOut;
}

void VoltageSrc::OnInput(struct avr_irq_t*, uint32_t value)
{
    m_fCurrentV = (float)value / 256.0f;
}

VoltageSrc::VoltageSrc(float fVScale,float fStart):m_fCurrentV(fStart), m_fVScale(fVScale)
{
}

void VoltageSrc::Init(struct avr_t * avr , uint8_t uiMux)
{
    _Init(avr,uiMux,this);
    RegisterNotify(VALUE_IN, MAKE_C_CALLBACK(VoltageSrc,OnInput), this);
	auto pTH = TelemetryHost::GetHost();
	pTH->AddTrace(this, ADC_VALUE_OUT, {TC::ADC, TC::Power},16);
	pTH->AddTrace(this, DIGITAL_OUT,{TC::Power,TC::InputPin});

}

// Sets the voltage readback to the given value, based on the scale factor
void VoltageSrc::Set(float fVal)
{
    uint32_t value = fVal * 256;
	RaiseIRQ(VALUE_IN, value);
}
