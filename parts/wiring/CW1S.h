/*
	CW1S.h - Wiring definition for the Prusa CW1S
	Copyright 2021 VintagePC <https://github.com/vintagepc/>

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

#include "PinSpec_32u4.h"
#include "Wiring.h"

namespace Wirings
{
	class CW1S : public Wiring
	{

		public:
			CW1S():Wiring(m_pSpec)
			{
				 m_mPins = GetPinMap();
			};

		protected:
			std::map<Pin,MCUPin> GetPinMap()
			{
				return {
					{ LCD_BL_PIN,  6},
					{ LCD_PINS_D4, 10},
					{ LCD_PINS_D5, 18},
					{ LCD_PINS_D6, 31},
					{ LCD_PINS_D7, 19},
					{ LCD_PINS_ENABLE, 4},
					{ LCD_PINS_RS, 12},
					{ BEEPER, 9},
					{ BTN_EN1, 20},
					{ BTN_EN2, 21},
					{ BTN_ENC, 0},
					{ X_STEP_PIN, 5},
					{ X_TMC2130_CS, 7},
					{ MCP_CSEL, 8},
					{ TACH_0 ,0},
					{ TACH_1, 2},
					{ TACH_2, 1},
					{ FAN_PIN, 13},
					{ FAN_1_PIN, 11},
					{ LED_PIN, 3}
				};
			};

		private:
			PinSpec_32u4 m_pSpec = PinSpec_32u4();
	};
}; // namespace Wirings
