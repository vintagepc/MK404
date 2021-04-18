/*
	EinsyRambo.h - Board definition for the Prusa EinsyRambo
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

#include "Beeper.h"                              // for Beeper
#include "Board.h"                               // for Board
#include "Button.h"                              // for Button
#include "Fan.h"                                 // for Fan
#include "HD44780GL.h"                           // for HD44780GL
#include "Heater.h"                              // for Heater
#include "IKeyClient.h"
#include "LED.h"                                 // for LED
#include "Macros.h"
#include "PINDA.h"                               // for PINDA
#include "RotaryEncoder.h"                       // for RotaryEncoder
#include "SDCard.h"                              // for SDCard
#include "SerialLineMonitor.h"                   // for SerialLineMonitor
#include "TMC2130.h"                             // for TMC2130
#include "Thermistor.h"                          // for Thermistor
#include "VoltageSrc.h"                          // for VoltageSrc
#include "sim_irq.h"                             // for avr_irq_t
#include "uart_pty.h"                            // for uart_pty
#include "w25x20cl.h"                            // for w25x20cl
#include "wiring/Einsy_1_1a.h"                   // for Einsy_1_1a
#include <cstdint>                              // for uint32_t

extern "C"
{
	#include "../3rdParty/MK3/Configuration_prusa.h" //NOLINT
	#undef MMU_HWRESET
}

namespace Boards
{
	class EinsyRambo: public Board, virtual private IKeyClient
	{
		public:
			explicit EinsyRambo(uint32_t uiFreq = 16000000);

		protected:
			void SetupHardware() override;

			void OnAVRReset() override;

			void OnAVRInit() override;

			void OnAVRDeinit() override;

			void OnKeyPress(const Key& key) override;

			void OnExtraHexChunk(gsl::span<uint8_t> chunk, uint32_t uiBase) override;

			static constexpr float fScale24v = 1.0f/26.097f; // Based on rSense voltage divider outputting 5v

			bool m_bFactoryReset = false;

			HD44780GL lcd;
			RotaryEncoder encoder;
			Button PowerPanic {"Power Panic",'p',"Triggers Power Panic line"};
			Beeper m_buzzer;
			uart_pty UART0, UART1, UART2;
			SerialLineMonitor m_Mon0 = SerialLineMonitor("Serial0");
			SerialLineMonitor m_Mon1 = SerialLineMonitor("Serial1");
			Thermistor tExtruder, tBed, tPinda, tAmbient;
			Fan fExtruder {3300,'E'}, fPrint {5000,'P',true};
			Heater hExtruder = {1.5,25.0,false,'H',30,250},
				hBed = {0.25, 25, true,'B',30,100};
			w25x20cl spiFlash;
			SDCard sd_card = SDCard();
			TMC2130 X {'X'},
				Y {'Y'},
				Z {'Z'},
				E {'E'};
			VoltageSrc vMain = VoltageSrc(fScale24v, 24.f),
				vBed = VoltageSrc(fScale24v,23.9);
			PINDA pinda { FL(X_PROBE_OFFSET_FROM_EXTRUDER), FL(Y_PROBE_OFFSET_FROM_EXTRUDER)};
			//MMU2 *mmu = nullptr;
			LED lPINDA {0xFF0000FF,'P',true},
				lIR {0xFFCC00FF,'I',true},
				lSD {0x0000FF00,'C', true};

		private:

			void DebugPin(avr_irq_t *irq, uint32_t value);

			const Wirings::Einsy_1_1a m_wiring = Wirings::Einsy_1_1a();
	};
}; // namespace Boards
