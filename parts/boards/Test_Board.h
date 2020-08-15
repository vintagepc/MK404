/*
	Test.h - Board definition for a "Test" board.
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

// #include "Beeper.h"                              // for Beeper
#include "Board.h"                               // for Board
// #include "Button.h"                              // for Button
// #include "Fan.h"                                 // for Fan
// #include "HD44780GL.h"                           // for HD44780GL
// #include "Heater.h"                              // for Heater
// #include "LED.h"                                 // for LED
// #include "PINDA.h"                               // for PINDA
#include "RotaryEncoder.h"                       // for RotaryEncoder
// #include "SDCard.h"                              // for SDCard
// #include "SerialLineMonitor.h"                   // for SerialLineMonitor
// #include "TMC2130.h"                             // for TMC2130
// #include "Thermistor.h"                          // for Thermistor
// #include "VoltageSrc.h"                          // for VoltageSrc
// #include "sim_irq.h"                             // for avr_irq_t
// #include "uart_pty.h"                            // for uart_pty
// #include "w25x20cl.h"                            // for w25x20cl
#include "wiring/Test_Wiring.h"                   // for Einsy_1_1a
#include <cstdint>                              // for uint32_t
#include <string>                                // for string


namespace Boards
{
	class Test_Board: public Board
	{
		public:
			Test_Board(uint32_t uiFreq = 16000000)
				:Board(m_wiring,uiFreq){ SetBoardName("Test_Board");};

		protected:
			void SetupHardware() override;

			void OnAVRReset() override;

			void OnAVRInit() override;

			void OnAVRDeinit() override;

			RotaryEncoder encoder;

		private:

			void DebugPin(avr_irq_t *irq, uint32_t value);

			const Wirings::Test_Wiring m_wiring = Wirings::Test_Wiring();
	};
};
