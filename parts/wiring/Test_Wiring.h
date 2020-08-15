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

#include "Wiring.h"
#include "PinSpec_2560.h"
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
				};
			};
		private:
			const PinSpec_2560 m_EinsyPins = PinSpec_2560();
	};
};
