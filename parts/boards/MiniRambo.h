/*
	MiniRambo.h - Board definition for the Prusa MiniRambo
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

#include "Board.h"                               // for Board
#include "A4982.h"
#include "HD44780GL.h"                           // for HD44780G
#include "RotaryEncoder.h"
#include "sim_irq.h"                             // for avr_irq_t
#include "uart_pty.h"                            // for uart_pty
#include "wiring/miniRAMBo_1_3a.h"                   // for Einsy_1_1a
#include <cstdint>                              // for uint32_t
#include <string>                                // for string

namespace Boards
{
	class MiniRambo: public Board
	{
		public:
			MiniRambo(uint32_t uiFreq = 16000000)
				:Board(m_wiring,uiFreq){ SetBoardName("miniRAMBo");};

			~MiniRambo(){};

		protected:
			void SetupHardware() override;

			void OnAVRReset() override;

			void OnAVRInit() override;

			void OnAVRDeinit() override;

			HD44780GL lcd;
			uart_pty UART0;
			RotaryEncoder encoder;
			A4982 X{'X'};


		private:

			void DebugPin(avr_irq_t *irq, uint32_t value);

			const Wirings::miniRAMBo_1_3a m_wiring = Wirings::miniRAMBo_1_3a();
	};
};
