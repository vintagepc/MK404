/*
	Thermistor.cpp

	Based on thermistor.c (C) 2008-2012 Michel Pollet <buserror@gmail.com>

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

#include <string.h>
#include <stdio.h>


#include "Thermistor.h"

Thermistor::Thermistor(float fStartTemp):m_fCurrentTemp(fStartTemp)
{

}

uint32_t Thermistor::OnADCRead(struct avr_irq_t * irq, uint32_t value)
{
	short *t = m_pTable, *lt = NULL;
	for (int ei = 0; ei < m_uiTableEntries; ei++, lt = t, t += 2) {
		if (t[1] <= m_fCurrentTemp) {
			short tt = t[0];
			/* small linear regression between table samples */
			if (ei > 0 && t[1] < m_fCurrentTemp) {
				short d_adc = t[0] - lt[0];
				float d_temp = t[1] - lt[1];
				float delta = m_fCurrentTemp - t[1];
				tt = t[0] + (d_adc * (delta / d_temp));
			}
			// if (m_adc_mux_number==-1)
			// 	printf("simAVR ADC out value: %u\n",((tt / m_oversampling) * 5000) / 0x3ff);
			uint32_t uiVal = (((tt / m_iOversampling) * 5000) / 0x3ff);
			return uiVal;
		}
	}
	printf("%s(%d) temperature out of range (%.2f), we're screwed\n",
			__func__, m_uiMux, m_fCurrentTemp);
	return UINT32_MAX;
}

void Thermistor::OnTempIn(struct avr_irq_t * irq, uint32_t value)
{
	float fv = ((float)value) / 256;
	m_fCurrentTemp = fv;

	RaiseIRQ(TEMP_OUT, value);
}

void Thermistor::Init(struct avr_t * avr, uint8_t uiMux)
{
	
	_Init(avr, uiMux,this);
	RegisterNotify(TEMP_IN,MAKE_C_CALLBACK(Thermistor,OnTempIn),this);
	printf("%s on ADC %d start %.2f\n", __func__, m_uiMux, m_fCurrentTemp);
}

void Thermistor::SetTable(short *pTable, unsigned int uiEntries, int iOversamp)
{
	m_iOversampling = iOversamp;
	m_pTable = pTable;
	m_uiTableEntries = uiEntries;
}

void Thermistor::Set(float fTempC)
{
	uint32_t value = fTempC * 256;
	m_fCurrentTemp = fTempC;

	RaiseIRQ(TEMP_OUT, value);
}
