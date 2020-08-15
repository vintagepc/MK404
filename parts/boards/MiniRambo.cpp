/*
	MiniRambo.cpp - Board definition for the Prusa MiniRambo
	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK404

	MK404is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK404is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK404  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MiniRambo.h"
#include "HD44780.h"           // for HD44780
#include "PinNames.h"          // for Pin, Pin::BTN_ENC, Pin::W25X20CL_PIN_CS
#include <iostream>

namespace Boards
{
	void MiniRambo::SetupHardware()
	{
		DisableInterruptLevelPoll(8);

		AddHardware(UART0);

		AddHardware(lcd);
		// D4-D7,
		PinNames::Pin ePins[4] = {LCD_PINS_D4,LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7};
		for (int i = 0; i < 4; i++) {
			TryConnect(ePins[i],lcd, HD44780::D4+i);
			TryConnect(lcd, HD44780::D4+i,ePins[i]);
		}
		TryConnect(LCD_PINS_RS,lcd, HD44780::RS);
		TryConnect(LCD_PINS_ENABLE, lcd,HD44780::E);

		AddHardware(encoder);
		TryConnect(encoder, RotaryEncoder::OUT_A, BTN_EN2);
		TryConnect(encoder, RotaryEncoder::OUT_B, BTN_EN1);
		TryConnect(encoder, RotaryEncoder::OUT_BUTTON, BTN_ENC);

		AddUARTTrace('0'); // External

		//avr_irq_register_notify(GetDIRQ(PAT_INT_PIN), MAKE_C_CALLBACK(EinsyRambo, DebugPin),this);

	}

	// Convenience function for debug printing a particular pin.
	void MiniRambo::DebugPin(avr_irq_t *irq, uint32_t value)
	{
		cout << "Pin DBG: change to " << std::hex << value << '\n';
	}

	void MiniRambo::OnAVRInit()
	{
	}

	void MiniRambo::OnAVRDeinit()
	{
	}

	void MiniRambo::OnAVRReset()
	{
		cout << "RESET\n";
		DisableInterruptLevelPoll(8);

		SetPin(BTN_ENC,1);

		UART0.Reset();
	}



};
