/*
	Test_Wiring.h - Pin definition for a 2560 "Test" board to test components.
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

#include "PinNames.h"
#include "PinSpec_2560.h"
#include "Wiring.h"

namespace Wirings
{

	class Test_Wiring : public Wiring
	{
		public:
			Test_Wiring():Wiring(m_EinsyPins)
			{
				m_mPins = GetPinMap();
			};

		protected:
			std::map<Pin,MCUPin> GetPinMap()
			{
				return {
					{BTN_EN1,72},
					{BTN_EN2,14},
					{BTN_ENC,9},
					{IR_SENSOR_PIN,54},
					{W25X20CL_PIN_CS,53},
					{VOLT_PWR_PIN,55},
					{BTN_ARRAY,56},
					{TEMP_0_PIN,57},
					{X_TMC2130_CS,22},
					{X_TMC2130_DIAG,23},
					{X_STEP_PIN,24},
					{X_DIR_PIN,25},
					{X_ENABLE_PIN,26},
					{SHIFT_CLOCK,27},
					{SHIFT_DATA,28},
					{SHIFT_LATCH,29},
					{FAN_PIN,6},
					{TACH_0,79},
					{HEATER_0_PIN,7},
					{Z_MIN_PIN,79},
					{LCD_PINS_D4,49},
					{LCD_PINS_D5,48},
					{LCD_PINS_D6,47},
					{LCD_PINS_D7,46},
					{LCD_PINS_ENABLE,45},
					{LCD_PINS_RS,44},
					{SDCARDDETECT,43},
					{SDSS,42},
					{SWI2C_SCL, 21},
					{SWI2C_SDA, 20},
					{LCD_BL_PIN, 8},
					{BEEPER, 5},
					{E_MUX0_PIN, 76},
					{E_MUX1_PIN, 77},
					{X_SLP_PIN, 74},
					{X_RST_PIN, 75},
					{X_MAX_PIN, 73}
				};
			};
		private:
			const PinSpec_2560 m_EinsyPins = PinSpec_2560();
	};
}; // namespace Wirings
