/*
	SL1.h - Board definition for the Prusa SL1 control board
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
#include "Heater.h"
#include "LED.h"
#include "MCP23S17.h"
#include "TMC2130.h"
#include "Thermistor.h"
#include "uart_pty.h"
#ifndef __APPLE__
extern "C"
{
	#include "usbip.h"
}
#endif
#include "wiring/SL1_Control.h"
#include <cstdint>                // for uint32_t

namespace Boards
{
	class SL1: public Board
	{
		public:
			explicit SL1(uint32_t uiFreq = 16000000)
				:Board(m_wiring,uiFreq){};

			~SL1() override;

			void Draw();

		protected:
			void SetupHardware() override;

		Button m_pwr {"Power",'p',"Power button"};
		Beeper m_beep;
		Fan m_f1 {5000,'1', false}, m_f2 {5000,'2', false},  m_f3 {500,'3', false};
		Heater m_htUV {0.05f, 20.f, false, 'U', 15.f, 150.f};
		MCP23S17 m_gpio;
		TMC2130 m_tower {'W'}, m_tilt {'T'};
		L74HCT4052 m_muxX, m_muxY;
		LED m_uv {0x8800FF00,'U'}, m_lpwr {0xFF880000, 'P'}, m_tx {0xFFCC0000,'X'}, m_rx {0x00FF0000,'R'};
		Thermistor m_tUV {20.f}, m_tAmb;

		uart_pty m_uart;

#ifndef __APPLE__
		pthread_t m_usb_thread = 0;
		usbip_t* m_usb = nullptr;
#endif

		private:
			const Wirings::SL1_Control m_wiring = Wirings::SL1_Control();

			std::string m_strTitle = "Sharks (with) Lasers (1) Sensual";
			// Yes, this is an Austin Powers and Makers Muse joke...
	};
}; // namespace Boards
