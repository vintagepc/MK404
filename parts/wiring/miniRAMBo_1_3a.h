/*
	miniRAMBo_1_3a.h - Pin definition for an miniRAMBo 1.3a
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
	https://github.com/prusa3d/Prusa-Firmware/blob/MK3/Firmware/pins_Rambo_1_3.h
	https://github.com/ultimachine/Mini-Rambo/blob/1.3a/board/Project%20Outputs%20for%20Mini-Rambo/Mini-Rambo.PDF
    
    Version 0.1     15 Aug 2020     3d-gussner

    Change log:
    15 Aug 2020     3d-gussner      Init
 */

#pragma once

#include <miniRMBo_1_0_a.h>
namespace Wirings
{
	class miniRAMBo_1_3a : public miniRMBo_1_0_a
	{
		public:
			miniRAMBo_1_3a():miniRMBo_1_0_a()
			{
				m_mPins = GetPinMap();
			};

		protected:
			std::map<Pin, MCUPin> GetPinMap() override {
				auto baseMap = miniRMBo_1_0_a::GetPinMap();
				baseMap[SWI2C_SCL] = 21;
				baseMap.erase(KILL_PIN);
				baseMap[BEEPER] = 84;
				baseMap[BTN_EN1] = 72;
				baseMap[BTN_EN2] = 14;
				baseMap[BTN_ENC] = 9;
				baseMap[LCD_PINS_D4] = 19;
				baseMap[LCD_PINS_D5] = 70;
				baseMap[LCD_PINS_D6] = 85;
				baseMap[LCD_PINS_D7] = 71;
				baseMap[LCD_PINS_ENABLE] = 18;
				baseMap[LCD_PINS_RS] = 82;
				baseMap[SDCARDDETECT] = 15;
				return baseMap;
			};
	};
};
