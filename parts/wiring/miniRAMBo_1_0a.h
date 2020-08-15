 /*
	miniRAMBo_1_0a.h - Pin definition for an Ultimachine miniRAMBo 1.0a
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

	Based on:
	https://github.com/prusa3d/Prusa-Firmware/blob/MK3/Firmware/pins_Rambo_1_0.h
	
    Version 0.1     15 Aug 2020     3d-gussner

    Change log:
    15 Aug 2020     3d-gussner      Init
*/
#pragma once

#include <Wiring.h>
#include <PinSpec_2560.h>
namespace Wirings
{
	class miniRAMBo_1_1b : public Wiring
	{
		public:
			miniRAMBo_1_1b():Wiring(m_miniRAMBoPins)
			{
				m_mPins = GetPinMap();
			};

		protected:
			virtual std::map<Pin,MCUPin> GetPinMap() override
			{
				return {
					{BEEPER,78},
					{BTN_EN1,80},
					{BTN_EN2,73},
					{BTN_ENC,21},
					{E0_DIR_PIN,43},
					{E0_ENABLE_PIN,26},
					{E0_STEP_PIN,34},
					{E0_MS1_PIN,65},
					{E0_MS2_PIN,66},
					{E0_FAN,8},
					{FAN_PIN,6}, 				// Print fan
					{HEATER_0_PIN,3},
					{HEATER_BED_PIN,4},
					{KILL_PIN,32},
					{LCD_PINS_D4,14},
					{LCD_PINS_D5,15},
					{LCD_PINS_D6,32},
					{LCD_PINS_D7,31},
					{LCD_PINS_ENABLE,5},
					{LCD_PINS_RS,38},
					{LED_PIN,13},
					{SDCARDDETECT,72},
					{SDSS,53},
					{SWI2C_SCL,84},
					{SWI2C_SDA,20}				//IR_SENSOR_PIN on MK2.5S			
					{TEMP_0_PIN,0},
					{TEMP_1_PIN,1},				//TEMP_PINDA_PIN for MK2.5+MK2.5S
					{TEMP_AMBIENT_PIN,6},
					{TEMP_BED_PIN,2},
					{X_DIR_PIN,48},
					{X_ENABLE_PIN,29},
					{X_MIN_PIN,12},
					{X_MAX_PIN,30},				//TACH_0 for MK2.5+MK2.5S
					{X_STEP_PIN,37},
					{X_MS1_PIN,40},
					{X_MS2_PIN,41},
					{Y_DIR_PIN,49},
					{Y_ENABLE_PIN,28},
					{Y_MIN_PIN,11},
					{Y_MAX_PIN,24},
					{Y_STEP_PIN,36},
					{Y_MS1_PIN,69},
					{Y_MS2_PIN,39},
					{Z_DIR_PIN,47},
					{Z_ENABLE_PIN,27},
					{Z_MIN_PIN,10},
					{Z_MAX_PIN,23},
					{Z_STEP_PIN,35},
					{Z_MS1_PIN,68},
					{Z_MS2_PIN,67},
					{TX2_PIN,17},				//E_MUX0_PIN for MMUv1
					{RX2_PIN,16},				//E_MUX1_PIN for MMUv1
					{XY_REF_PIN,46},			//MOTOR_CURRENT_PWM_XY_PIN
					{Z_REF_PIN,45},				//MOTOR_CURRENT_PWM_Z_PIN
					{E_REF_PIN,44}				//MOTOR_CURRENT_PWM_E_PIN
				};
			};
		private:
			const PinSpec_2560 m_miniRAMBoPins = PinSpec_2560();
	};
};
