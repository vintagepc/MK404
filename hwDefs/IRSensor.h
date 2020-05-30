/*
	IRSensor.h - a simulated 3/S IR sensor, v0.4

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


#ifndef __IRSENSOR_H___
#define __IRSENSOR_H___

#include "VoltageSrc.h"

class IRSensor: public VoltageSrc 
{
public:
	// Enumeration for IR sensor states.
	typedef enum IRState {
		IR_SHORT,
		IR_FILAMENT_PRESENT,
		IR_UNKNOWN,
		IR_NO_FILAMENT,
		IR_NOT_CONNECTED,
		IR_AUTO // Special state that only respects the auto value.
	}IRState;

	// Constructs a new IRSensor on ADC mux uiMux
    IRSensor();


	// Flips the state between filament and no filament. 
	void Toggle();

	// Sets the sensor output to a given state.
	void Set(IRState eVal);

	// Consumer for external (auto) sensor hook, set 0 or 1 to signify absence or presence of filament.
	void Auto_Input(uint32_t val);

private:
	// ADC read trigger
 	uint32_t OnADCRead(avr_irq_t *pIRQ, uint32_t value) override;

	// LUT for states to voltage readouts.
	float m_fIRVals[IR_AUTO] = 
	{
		[IR_SHORT] = 0.1f,
		[IR_FILAMENT_PRESENT] = 0.4f,
		[IR_UNKNOWN] = 3.0f,
		[IR_NO_FILAMENT] = 4.5f,
		[IR_NOT_CONNECTED] = 4.9f
	};

	bool m_bExternal = false; 
	IRState m_eCurrent = IR_NO_FILAMENT;
};

#endif /* __IRSENSOR_H___ */
