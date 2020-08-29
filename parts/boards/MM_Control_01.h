/*
	MM_Control_01.h - Board definition for the Prusa MMU2
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

#include "ADC_Buttons.h"           // for ADC_Buttons
#include "Board.h"                 // for Board
#include "HC595.h"                 // for HC595
#include "LED.h"                   // for LED
#include "MM_Control_01.h"
#include "TMC2130.h"               // for TMC2130
#include "uart_pty.h"              // for uart_pty
#include "wiring/MM_Control_01.h"  // for MM_Control_01
#include <cstdint>                // for uint32_t

namespace Boards
{
	class MM_Control_01: public Board
	{
		public:
			explicit MM_Control_01(uint32_t uiFreq = 16000000)
				:Board(m_wiring,uiFreq){};

			~MM_Control_01() override = default;

		protected:
			void SetupHardware() override;

		//	void CustomAVRInit() override;

		//	void CustomAVRDeinit() override;

			uart_pty m_UART;
			HC595 m_shift;
			TMC2130 m_Sel {'S'},
					m_Idl {'I'},
					m_Extr {'P'};
			LED m_lGreen[5] {{0x00FF00FF,' '},{0x00FF00FF, ' '},{0x00FF00FF, ' '},{0x00FF00FF, ' '},{0x00FF00FF,' '}},
				m_lRed[5] {{0xFF0000FF,' '},{0xFF0000FF,' '},{0xFF0000FF,' '},{0xFF0000FF,' '},{0xFF0000FF,' '}},
				m_lFINDA {0xFFCC00FF,'F'};
			ADC_Buttons m_buttons {"MMUButtons"};

		private:
			const Wirings::MM_Control_01 m_wiring = Wirings::MM_Control_01();
	};
}; // namespace Boards
