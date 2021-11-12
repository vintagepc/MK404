/*
	SL1_Control.h - Wiring definition for the Prusa SL1_Control board
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
	class SL1_Control : public Wiring
	{

		public:
			SL1_Control():Wiring(m_pSpec)
			{
				 m_mPins = GetPinMap();
			};

		protected:
			std::map<Pin,MCUPin> GetPinMap()
			{
				return {
					{ BEEPER, 5},
					{ Z_STEP_PIN, 9},
					{ Y_STEP_PIN, 10},
					{ MCP_CSEL, 8 },
					{ LED_PIN, 11},
					{ LED_1_PIN, 3},
					{ Z_TMC2130_CS, 2},
					{ Y_TMC2130_CS, 4},
					{ TX_LED, 30},
					{ RX_LED, 17},
					{ Z_TMC2130_DIAG, 12},
					{ Y_TMC2130_DIAG, 6},
					{ TACH_0, 17},
					{ TACH_1, 16},
					{ TACH_2, 15},
					{ FAN_2_PIN, 14}
				};
			};

		private:
			PinSpec_32u4 m_pSpec = PinSpec_32u4();
	};
}; // namespace Wirings
