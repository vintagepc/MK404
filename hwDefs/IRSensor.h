/*

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


#ifndef __IRSENSOR_H___
#define __IRSENSOR_H___

#include "VoltageSrc.h"

class IRSensor: public VoltageSrc 
{
public:
	typedef enum IRState {
		IR_SHORT,
		IR_FILAMENT_PRESENT,
		IR_UNKNOWN,
		IR_NO_FILAMENT,
		IR_NOT_CONNECTED,
		IR_AUTO // Special state that only respects the auto value.
	}IRState;

    IRSensor(uint8_t uiMux);

 	void OnADCRead(avr_irq_t *pIRQ, uint32_t value) override;

	// Flips the state between filament and no filament. 
	void Toggle();

	// Sets the sensor output to a given state.
	void Set(IRState eVal);

	// Consumer for external (auto) sensor hook, set 0 or 1 to signify absence or presence of filament.
	void Auto_Input(uint32_t val);

private:

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
