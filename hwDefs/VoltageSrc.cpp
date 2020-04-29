/*
	VoltageSrc.cpp

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
#include <stdio.h>
#include "Util.h"
#include "avr_adc.h"
#include "VoltageSrc.h"


void VoltageSrc::OnADCRead(struct avr_irq_t * irq, uint32_t value)
{
	union {
		avr_adc_mux_t v;
		uint32_t l;
	} u = { .l = value };
	avr_adc_mux_t v = u.v;

    if (v.src != m_uiMuxNr)
		return;

    uint32_t iVOut =  (m_fCurrentV*m_fVScale)*1000*5;
	SendToADC(iVOut);
}

void VoltageSrc::SendToADC(uint32_t uiVOut)
{
	avr_raise_irq(m_pIrq + ADC_VALUE_OUT,uiVOut);
	if (uiVOut>2200) // 2.2V, logic H
        avr_raise_irq(m_pIrq + DIGITAL_OUT,1);
	else if (uiVOut < 800) // 0.8v. L
        avr_raise_irq(m_pIrq + DIGITAL_OUT,0);
	else
        avr_raise_irq_float(m_pIrq + DIGITAL_OUT,0,1);
    return;
}

void VoltageSrc::OnInput(struct avr_irq_t *irq, uint32_t value)
{
    m_fCurrentV = (float)value / 256.0f;
}

VoltageSrc::VoltageSrc(uint8_t adc_mux_number, float fVScale,float fStart):m_fCurrentV(fStart), m_fVScale(fVScale), m_uiMuxNr(adc_mux_number)
{
}

void VoltageSrc::Init(struct avr_t * avr )
{
    _Init(avr,this);
	
    RegisterNotify(ADC_TRIGGER_IN, MAKE_C_CALLBACK(VoltageSrc,OnADCRead), this);
    RegisterNotify(VALUE_IN, MAKE_C_CALLBACK(VoltageSrc,OnInput), this);
    avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER);
    avr_irq_t * dst = avr_io_getirq(m_pAVR, AVR_IOCTL_ADC_GETIRQ, m_uiMuxNr);
	if (src && dst) {
		ConnectFrom(src, ADC_TRIGGER_IN);
        ConnectTo(ADC_VALUE_OUT, dst);
	}
}

// Sets the voltage readback to the given value, based on the scale factor
void VoltageSrc::Set(float fVal)
{
    uint32_t value = fVal * 256;
	RaiseIRQ(VALUE_IN, value);
}
