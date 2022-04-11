/*
	Thermistor.cpp

	Based on thermistor.c (C) 2008-2012 Michel Pollet <buserror@gmail.com>

	Rewritten for C++ in 2020 by VintagePC <https://github.com/vintagepc/>

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

#include "Thermistor.h"
#include "BasePeripheral.h"  // for MAKE_C_CALLBACK
#include "TelemetryHost.h"
#include <algorithm>         // for copy, max
#include <iostream>
#include <iterator>

Thermistor::Thermistor(float fStartTemp):Scriptable("Thermistor"),m_fCurrentTemp(fStartTemp)
{
	RegisterActionAndMenu("Disconnect","Disconnects the thermistor as though it has gone open circuit",Actions::OpenCircuit);
	RegisterActionAndMenu("Short","Short the thermistor out",Actions::Shorted);
	RegisterActionAndMenu("Reconnect","Restores the normal thermistor state",Actions::Connected);
	RegisterActionAndMenu("BadPullUp","Act as though the internal pullup on the Atmega is enabled",Actions::BadPullup);
	RegisterActionAndMenu("BadFerrite","Act as though the ferrite in series with the thermistor is bad",Actions::BadFerrite);
	RegisterActionAndMenu("BadFerriteAndPullup","Act as though the ferrite in series with the thermistor is bad, together with bad pullup",Actions::BadFerriteAndPullup);
	RegisterAction("Set", "Sets the temperature to the specified value", ActSetTemp, {ArgType::Float});
}


Scriptable::LineStatus Thermistor::ProcessAction(unsigned int iAction, const std::vector<std::string>& vArgs)
{
	switch (iAction)
	{
		case ActSetTemp:
		{
			Set(std::stof(vArgs.at(0)));
			return LineStatus::Finished;
		}
		case Shorted:
		case OpenCircuit:
		case Connected:
		case BadPullup:
		case BadFerriteAndPullup:
		case BadFerrite:
		{
			m_eState = static_cast<Actions>(iAction);
			return LineStatus::Finished;
		}
		default:
			return LineStatus::Unhandled;
	}
}

static constexpr float fVcc = 5; // mV
static constexpr float fRs = 2370; // Ohms
static constexpr float fRPu = 4700; // Ohms
static constexpr float fRFerrite = 650; // Ohms - resistance of a bad ferrite
static constexpr float fRIntPu = 40000; // internal pullup (avg 20K-55K)
static constexpr float fRUpper = 1.f/((1.f/(fRIntPu+(2*fRs)))+(1.f/(fRPu)));

float Thermistor::CalcBadPullup(float fNorm)
{

	// Calculate something proportional to Rtherm based on normal measurement:
	float fRth = (fNorm*fRPu)/(fVcc-fNorm);
	// If bad ferrite, add it to the Th resistance
	if (m_eState == BadFerriteAndPullup || m_eState == BadFerrite)
	{
		fRth += fRFerrite;
	}
	// Voltage at midpoint junction between therm and 4.7K pullup
	float fVx = (fVcc*(fRth))/(fRUpper + fRth);
	// New measured value at ADC input - if no pullup enabled.
	float fNew = fVx;
	// If bad pullup, do the additional divider calc:
	if (m_eState == BadFerriteAndPullup || m_eState == BadPullup)
	{
		 fNew += ((fVcc-fVx)*((2*fRs)/((2*fRs)+fRIntPu)));
	}
	return fNew;
}

uint32_t Thermistor::OnADCRead(struct avr_irq_t*, uint32_t)
{
	if (m_eState == Shorted)
	{
		return 0;
	}
	else if (m_eState == OpenCircuit)
	{
		return 5000;
	}

	for (auto it = m_vTable.begin(); it!= m_vTable.end(); it++) {
		if (it->second <= m_fCurrentTemp) {
			int16_t tt = it->first;
			/* small linear regression between table samples */
			if (it!=m_vTable.begin() && it->second < m_fCurrentTemp) {
				int16_t d_adc = it->first - std::prev(it)->first;
				float d_temp = it->second - std::prev(it)->second;
				float delta = m_fCurrentTemp - it->second;
				tt = it->first + (d_adc * (delta / d_temp));
			}
			// if (m_adc_mux_number==-1)
			// 	printf("simAVR ADC out value: %u\n",((tt / m_oversampling) * 5000) / 0x3ff);
			uint32_t uiVal = (tt / m_iOversampling);
			if (m_eState >= BadPullup && m_eState <= BadFerriteAndPullup)
			{
				return  1000.f*CalcBadPullup(5.f*static_cast<float>(uiVal)/1023.f);
			}
			else
			{
				return (uiVal* 5000) / 0x3ff;
			}
		}
	}
	std::cout << static_cast<const char*>(__FUNCTION__) << '(' << std::to_string(GetMuxNumber()) << ") temperature out of range: " << m_fCurrentTemp << '\n';
	return UINT32_MAX;
}

void Thermistor::OnTempIn(struct avr_irq_t *, uint32_t value)
{
	float fv = static_cast<float>(value) / 256.f;
	m_fCurrentTemp = fv;

	RaiseIRQ(TEMP_OUT, value);
}

void Thermistor::Init(struct avr_t * avr, uint8_t uiMux)
{

	_Init(avr, uiMux,this);
	RegisterNotify(TEMP_IN,MAKE_C_CALLBACK(Thermistor,OnTempIn),this);
	std::cout << static_cast<const char*>(__FUNCTION__) << " on ADC " << std::to_string(GetMuxNumber()) << " start temp: " <<m_fCurrentTemp << '\n';

	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, ADC_VALUE_OUT, {TC::ADC,TC::Thermistor}, 16);
	TH.AddTrace(this, TEMP_OUT, {TC::Thermistor,TC::Misc},16);
}

void Thermistor::SetTable(const gsl::span<const int16_t> table, int iOversamp)
{
	m_iOversampling = iOversamp;
	m_vTable.clear();
	for (auto it = table.begin(); it!= table.end(); it+=2)
	{
		m_vTable.push_back(std::make_pair(*it, *std::next(it)));
	}
}

void Thermistor::Set(float fTempC)
{
	uint32_t value = fTempC * 256;
	RaiseIRQ(TEMP_IN,value);
}
