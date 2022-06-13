/*
	CW1S.h - Board definition for the Prusa CW1/S
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

#include "74HCT4052.h"
#include "Beeper.h"
#include "Board.h"                 // for Board
#include "Button.h"
#include "Fan.h"
#include "HD44780GL.h"
#include "Heater.h"
#include "LED.h"
#include "MCP23S17.h"
#include "RotaryEncoder.h"
#include "TMC2130.h"
#include "Thermistor.h"
#ifndef __APPLE__
extern "C"
{
	#include "usbip.h"
}
#endif
#include "wiring/CW1S.h"  // for MM_Control_01
#include <cstdint>                // for uint32_t

namespace Boards
{
	class CW1S: public Board
	{
		public:
			explicit CW1S(uint32_t uiFreq = 16000000)
				:Board(m_wiring,uiFreq){};

			~CW1S() override;

			void Draw();

		protected:
			void SetupHardware() override;

			void SetIsCW1S(bool bVal) { m_bIsCW1S = bVal; }


		//	void CustomAVRInit() override;

		//	void CustomAVRDeinit() override;

		Button m_lid {"Lid",'l',"Opens/closes the lid"}, m_tank {"Tank",'t',"Inserts/removes the IPA tank"};
		Beeper m_beep;
		Fan m_f1 {5000,'1', false}, m_f2 {5000,'2', false},  m_f3 {500,'3', false};
		HD44780GL m_lcd;
		Heater m_ht {0.4, 25.f, false, 'H', 25.f, 150.f};
		Heater m_htUV {0.05f, 20.f, false, 'U', 20.f, 150.f};
		MCP23S17 m_gpio;
		RotaryEncoder m_enc;
		TMC2130 m_tmc {'W'};
		L74HCT4052 m_muxX, m_muxY;
		LED m_uv {0x8800FF00,'U'};
		Thermistor m_tUV {20.f}, m_tAmb;

#ifndef __APPLE__
		pthread_t m_usb_thread = 0;
		usbip_t* m_usb = nullptr;
#endif

		private:
			const Wirings::CW1S m_wiring = Wirings::CW1S();

			bool m_bIsCW1S = true;

			std::string m_strTitle = "Curiosity for Washing 1 (Secret)";
	};
}; // namespace Boards
