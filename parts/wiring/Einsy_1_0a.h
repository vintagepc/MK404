/*
	Einsy_1_0a.h - Pin definition for an Einsy 1.0a
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

#pragma once

#include <Wiring.h>
#include <PinSpec_2560.h>
namespace Wirings
{
	class Einsy_1_0a : public Wiring
	{
		public:
			Einsy_1_0a():Wiring(m_EinsyPins)
			{
				m_mPins = GetPinMap();
			};

		protected:
			virtual std::map<Pin,MCUPin> GetPinMap() override
			{
				return {
					{BEEPER,84},
					{BTN_EN1,72},
					{BTN_EN2,14},
					{BTN_ENC,9},
					{E0_DIR_PIN,43},
					{E0_ENABLE_PIN,26},
					{E0_STEP_PIN,34},
					{E0_TMC2130_CS,66},
					{E0_TMC2130_DIAG,65},
					{E0_FAN,8},
					{FAN_PIN,6}, // Print fan
					{HEATER_0_PIN,3},
					{HEATER_BED_PIN,4},
					{KILL_PIN,32},
					{LCD_PINS_D4,59},
					{LCD_PINS_D5,70},
					{LCD_PINS_D6,85},
					{LCD_PINS_D7,71},
					{LCD_PINS_ENABLE,61},
					{LCD_PINS_RS,82},
					{LED_PIN,13},
					{SDCARDDETECT,15},
					{SDSS,77},
					{TEMP_0_PIN,0},
					{TEMP_1_PIN,1},
					{TEMP_AMBIENT_PIN,6},
					{TEMP_BED_PIN,2},
					{X_DIR_PIN,49},
					{X_ENABLE_PIN,29},
					{X_MIN_PIN,12},
					{X_STEP_PIN,37},
					{X_TMC2130_CS,41},
					{X_TMC2130_DIAG,64},
					{Y_DIR_PIN,48},
					{Y_ENABLE_PIN,28},
					{Y_MIN_PIN,11},
					{Y_STEP_PIN,36},
					{Y_TMC2130_CS,39},
					{Y_TMC2130_DIAG,69},
					{Z_DIR_PIN,47},
					{Z_ENABLE_PIN,27},
					{Z_MIN_PIN,10},
					{Z_STEP_PIN,35},
					{Z_TMC2130_CS,67},
					{Z_TMC2130_DIAG,68}
				};
			};
		private:
			const PinSpec_2560 m_EinsyPins = PinSpec_2560();
	};
};
