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

#include "A4982.h"
#include "Beeper.h"
#include "Board.h"                               // for Board
#include "Fan.h"
#include "HD44780GL.h"                           // for HD44780G
#include "Heater.h"
#include "LED.h"
#include "MMU1.h"
#include "PINDA.h"
#include "RotaryEncoder.h"
#include "SDCard.h"
#include "SerialLineMonitor.h"
#include "Thermistor.h"
#include "sim_irq.h"                             // for avr_irq_t
#include "uart_pty.h"                            // for uart_pty
#include "wiring/miniRAMBo_1_3a.h"                   // for Einsy_1_1a
#include <cstdint>                              // for uint32_t

namespace Boards
{
	class MiniRambo: public Board
	{
		public:
			explicit MiniRambo(uint32_t uiFreq = 16000000)
				:Board(m_wiring,uiFreq){ SetBoardName("miniRAMBo");};

			~MiniRambo() override = default;

		protected:
			void SetupHardware() override;

			void OnAVRReset() override;

			void OnAVRInit() override;

			void OnAVRDeinit() override;

			HD44780GL lcd;
			Beeper m_buzzer;
			uart_pty UART0;
			RotaryEncoder encoder;
			A4982 X{'X'}, Y{'Y'}, Z{'Z'}, E{'E'}, E1{'2'}, E2{'3'}, E3{'4'};
			Thermistor tExtruder, tBed, tPinda, tAmbient;
			Fan fExtruder {3300,'E'}, fPrint {5000,'P'};
			Heater hExtruder {1.5,25.0,false,'H',30,250},
					hBed {0.25, 25, false,'B',30,100};
			PINDA pinda { (23.f), (9.f), PINDA::XYCalMap::MK2};
			LED lPINDA {0xFF0000FF,'P',true},
				lIR {0xFFCC00FF,'I',true},
				lSD {0x0000FF00,'C', true};
			SDCard sd_card{};

			MMU1 m_mmu;

			SerialLineMonitor m_Mon0 {"Serial0"};

		private:

			void DebugPin(avr_irq_t *irq, uint32_t value);

			const Wirings::miniRAMBo_1_3a m_wiring = Wirings::miniRAMBo_1_3a();
	};
}; // namespace Boards
