/*
	MM_Control_01.h - Wiring definition for the Prusa MMU2
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

#include "Wiring.h"
#include "PinSpec_32u4.h"
namespace Wirings
{
	class MM_Control_01 : public Wiring
	{

		public:
			MM_Control_01():Wiring(m_pSpec)
			{
				 m_mPins = GetPinMap();
			};

		protected:
			virtual std::map<Pin,MCUPin> GetPinMap() override
			{
				return {
					{BTN_ARRAY,2},
					{FINDA_PIN,19},
					{I_STEP_PIN,12},
					{I_TMC2130_CS,11},
					{I_TMC2130_DIAG,23},
					{P_STEP_PIN,8},
					{P_TMC2130_CS,5},
					{P_TMC2130_DIAG,21},
					{S_STEP_PIN,4},
					{S_TMC2130_CS,6},
					{S_TMC2130_DIAG,22},
					{SHIFT_CLOCK,13},
					{SHIFT_DATA,9},
					{SHIFT_LATCH,10},
				};
			};

		private:
			PinSpec_32u4 m_pSpec = PinSpec_32u4();
	};
};
