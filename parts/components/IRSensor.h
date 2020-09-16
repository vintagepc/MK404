/*
	IRSensor.h - a simulated 3/S IR sensor, v0.4

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

#include "IScriptable.h"  // for IScriptable::LineStatus
#include "VoltageSrc.h"   // for VoltageSrc
#include "sim_irq.h"      // for avr_irq_t
#include <atomic>
#include <cstdint>       // for uint32_t
#include <map>            // for map
#include <string>         // for string
#include <vector>         // for vector


class IRSensor: public VoltageSrc
{
public:
	// Enumeration for IR sensor states.
	using IRState = enum {
		IR_MIN = -1,
		IR_SHORT,
		IR_v3_FILAMENT_PRESENT,
		IR_v4_FILAMENT_PRESENT,
		IR_UNKNOWN,
		IR_v3_NO_FILAMENT,
		IR_v4_NO_FILAMENT,
		IR_NOT_CONNECTED,
		IR_AUTO, // Special state that only respects the auto value.
		IR_MAX
	};

	// Constructs a new IRSensor on ADC mux uiMux
    IRSensor();

	// Flips the state between filament and no filament.
	void Toggle();

	// Sets the sensor output to a given state.
	void Set(IRState eVal);

	// Consumer for external (auto) sensor hook, set 0 or 1 to signify absence or presence of filament.
	void Auto_Input(uint32_t val);

	protected:
		LineStatus ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs) override;

private:

	enum Actions
	{
		ActToggle = VoltageSrc::ActVS_END,
		ActSet,
		ActSetV3NoFilament,
		ActSetV3Filament,
		ActSetV4NoFilament,
		ActSetV4Filament,
		ActSetUnknown,
		ActSetAuto
	};

	// ADC read trigger
 	uint32_t OnADCRead(avr_irq_t *pIRQ, uint32_t value) override;

	// LUT for states to voltage readouts.
	std::map<IRState,float> m_mIRVals =
	{
		{IR_SHORT,0.1f},
		{IR_v4_FILAMENT_PRESENT,0.4f},
		{IR_v3_FILAMENT_PRESENT,0.2f},
		{IR_UNKNOWN, 3.0f},
		{IR_v4_NO_FILAMENT, 4.5f},
		{IR_v3_NO_FILAMENT, 4.7f},
		{IR_NOT_CONNECTED, 4.9}
	};

	std::atomic_bool m_bExternal {false};
	IRState m_eCurrent = IR_v4_NO_FILAMENT;
};
