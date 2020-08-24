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

#include <stdint.h>       // for uint32_t
#include <map>            // for map
#include <string>         // for string
#include <type_traits>    // for __decay_and_strip<>::__type
#include <atomic>
#include <utility>        // for make_pair, pair
#include <vector>         // for vector
#include "IScriptable.h"  // for IScriptable::LineStatus
#include "Scriptable.h"   // for Scriptable
#include "VoltageSrc.h"   // for VoltageSrc
#include "sim_irq.h"      // for avr_irq_t


class IRSensor: public VoltageSrc
{
public:
	// Enumeration for IR sensor states.
	typedef enum IRState {
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
	}IRState_t;

	// Constructs a new IRSensor on ADC mux uiMux
    IRSensor();


	// Flips the state between filament and no filament.
	void Toggle();

	// Sets the sensor output to a given state.
	void Set(IRState_t eVal);

	// Consumer for external (auto) sensor hook, set 0 or 1 to signify absence or presence of filament.
	void Auto_Input(uint32_t val);

	protected:
		LineStatus ProcessAction(unsigned int iAct, const vector<string> &vArgs) override;

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
	map<IRState_t,float> m_mIRVals =
	{
		make_pair(IR_SHORT,0.1f),
		make_pair(IR_v4_FILAMENT_PRESENT,0.4f),
		make_pair(IR_v3_FILAMENT_PRESENT,0.2f),
		make_pair(IR_UNKNOWN, 3.0f),
		make_pair(IR_v4_NO_FILAMENT, 4.5f),
		make_pair(IR_v3_NO_FILAMENT, 4.7f),
		make_pair(IR_NOT_CONNECTED, 4.9)
	};

	atomic_bool m_bExternal {false};
	IRState_t m_eCurrent = IR_v4_NO_FILAMENT;
};
