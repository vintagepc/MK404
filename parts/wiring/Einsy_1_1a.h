/*
	Einsy_1_1a.h - Pin definition for an Einsy 1.1a
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

#include <Einsy_1_0a.h>
namespace Wirings
{
	class Einsy_1_1a : public Einsy_1_0a
	{
		public:
			Einsy_1_1a():Einsy_1_0a()
			{
				m_mPins = GetPinMap();
			};

		protected:
			std::map<Pin, MCUPin> GetPinMap() override {
				auto baseMap = Einsy_1_0a::GetPinMap();
				baseMap[IR_SENSOR_PIN] = 62;
				baseMap.erase(KILL_PIN);
				baseMap[LCD_BL_PIN] = 5;
				baseMap[MMU_HWRESET] = 76;
				baseMap[TACH_0] = 79;
				baseMap[TACH_1] = 80;
				baseMap[TEMP_PINDA_PIN] = 3;
				baseMap[UVLO_PIN] = 2;
				baseMap[VOLT_BED_PIN] = 9;
				baseMap[VOLT_IR_PIN] = 8;
				baseMap[VOLT_PWR_PIN] = 4;
				baseMap[W25X20CL_PIN_CS] = 32;
				return baseMap;
			};
	};
};
