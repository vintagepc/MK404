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
#include "ADC_Buttons.h"
#include "Board.h"                               // for Board
#include "Button.h"                              // for Button
#include "Fan.h"                                 // for Fan
#include "GCodeSniffer.h"
#include "HC595.h"
#include "HD44780GL.h"                           // for HD44780GL
#include "Heater.h"                              // for Heater
// #include "LED.h"                                 // for LED
#include "IRSensor.h"                          // for IRSensor
#include "PAT9125.h"
#include "PINDA.h"                               // for PINDA
#include "RotaryEncoder.h"                       // for RotaryEncoder
#include "SDCard.h"                              // for SDCard
#include "SerialLineMonitor.h"                   // for SerialLineMonitor
#include "TMC2130.h"                             // for TMC2130
#include "Thermistor.h"                          // for Thermistor
#include "VoltageSrc.h"
//#include "uart_pty.h"                            // for uart_pty
#include "sim_irq.h"                             // for avr_irq_t
#include "w25x20cl.h"                            // for w25x20cl
#include "wiring/Test_Wiring.h"                   // for Einsy_1_1a
#include <cstdint>                              // for uint32_t


namespace Boards
{
	class Test_Board: public Board
	{
		public:
			explicit Test_Board(uint32_t uiFreq = 16000000)
				:Board(m_wiring,uiFreq){ SetBoardName("Test_Board");};

		protected:
			void SetupHardware() override;

			void OnAVRReset() override;

			void OnAVRInit() override;

			void OnAVRDeinit() override;

			RotaryEncoder encoder;

			Button m_btn {};

			SerialLineMonitor m_Monitor {"Serial0"};

			IRSensor m_IR;

			w25x20cl m_spiFlash;

			VoltageSrc m_vSrc {0.2f, 0.f };

			ADC_Buttons m_btns {"ADCButtons"};

			Thermistor m_thrm {25.f};

			TMC2130 m_TMC {'X'};

			HC595 m_shift{};

			Heater m_heat {5.f, 25.f, false, 'B',10.f,50.f};

			Fan m_Fan {2000,'F'};

			GCodeSniffer m_Sniff{'T'};

			PINDA m_pinda{0.f,0.f};

			HD44780GL m_lcd;

			SDCard m_card {};

			PAT9125 m_pat{};

		private:

			void DebugPin(avr_irq_t *irq, uint32_t value);

			const Wirings::Test_Wiring m_wiring = Wirings::Test_Wiring();
	};
}; // namespace Boards
