/*
	IRSensor.cpp

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

#include "IRSensor.h"
#include <iostream>  // for printf

// ADC read trigger.
uint32_t IRSensor::OnADCRead(struct avr_irq_t *, uint32_t)
{
    float fVal;
    if (m_eCurrent != IR_AUTO)
        fVal = m_mIRVals[m_eCurrent];
    else if (m_bExternal)
        fVal = m_mIRVals[IR_v4_FILAMENT_PRESENT];
    else
	{
        fVal = m_mIRVals[IR_v4_NO_FILAMENT];
	}

	uint32_t iVOut =  (fVal)*1000;

	return iVOut;
}

IRSensor::IRSensor():VoltageSrc(),Scriptable("IRSensor")
{
	RegisterActionAndMenu("Toggle","Toggles the IR sensor state",ActToggle);
	RegisterAction("Set","Sets the sensor state to a specific enum entry. (int value)",ActSet,{ArgType::Int});
	RegisterMenu("v0.4 Set Filament", ActSetV4Filament);
	RegisterMenu("v0.4 No Filament", ActSetV4NoFilament);
	RegisterMenu("v0.3 Set Filament", ActSetV3Filament);
	RegisterMenu("v0.3 No Filament", ActSetV3NoFilament);
	RegisterMenu("Set Unknown", ActSetUnknown);
}

Scriptable::LineStatus IRSensor::ProcessAction(unsigned int iAct, const vector<string> &vArgs)
{
	switch (iAct)
	{
		case ActToggle:
			Toggle();
			return LineStatus::Finished;
		case ActSet:
		{
			int iVal = stoi(vArgs.at(0));
			if (iVal<=IR_MIN || iVal >= IR_MAX)
				return IssueLineError(string("Set value ") + to_string(iVal) + " is out of the range [" + to_string(IR_MIN+1) + "," + to_string(IR_MAX-1) + "]" );
			Set((IRState)iVal);
			return LineStatus::Finished;
		}
		case ActSetV3Filament:
			Set(IR_v3_FILAMENT_PRESENT);
			return LineStatus::Finished;
		case ActSetV4Filament:
			Set(IR_v4_FILAMENT_PRESENT);
			return LineStatus::Finished;
		case ActSetV3NoFilament:
			Set(IR_v3_NO_FILAMENT);
			return LineStatus::Finished;
		case ActSetV4NoFilament:
			Set(IR_v4_NO_FILAMENT);
			return LineStatus::Finished;
		case ActSetUnknown:
			Set(IR_UNKNOWN);
			return LineStatus::Finished;
		case ActSetAuto:
			Set(IR_AUTO);
			return LineStatus::Finished;
	}
	return LineStatus::Unhandled;
}

// Default behaviour is V4
void IRSensor::Toggle()
{
	if (m_eCurrent == IR_AUTO)
		cout << "NOTE: Overriding IR Auto setting!" << endl;

	if (m_eCurrent == IR_v4_NO_FILAMENT)
	{
		cout << "IRSensor: Filament present!" << endl;
		m_eCurrent = IR_v4_FILAMENT_PRESENT;
	}
	else
	{
		cout << "IRSensor: No filament present!" << endl;
		m_eCurrent = IR_v4_NO_FILAMENT;
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
