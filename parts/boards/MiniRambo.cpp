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
#include "thermistortables.h"  // for OVERSAMPLENR, temptable_1, temptable_2000
#include <iostream>

#define TEMP_SENSOR_0 5
#define TEMP_SENSOR_BED 1
#define TEMP_SENSOR_AMBIENT 2000

#define _TERMISTOR_TABLE(num) \
		temptable_##num
#define TERMISTOR_TABLE(num) \
		_TERMISTOR_TABLE(num)


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

		AddHardware(tExtruder,GetPinNumber(TEMP_0_PIN));
		tExtruder.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_0),
								sizeof(TERMISTOR_TABLE(TEMP_SENSOR_0)) / sizeof(short) / 2,
								OVERSAMPLENR);

		AddHardware(tBed,GetPinNumber(TEMP_BED_PIN));
		tBed.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_BED),
							sizeof(TERMISTOR_TABLE(TEMP_SENSOR_BED)) / sizeof(short) / 2,
							OVERSAMPLENR);

		AddHardware(tAmbient,  GetPinNumber(TEMP_AMBIENT_PIN));
		tAmbient.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT),
		 						sizeof(TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT)) / sizeof(short) / 2,
		 						OVERSAMPLENR);

		AddHardware(X);
		TryConnect(X,A4982::DIR_IN,		X_DIR_PIN);
		TryConnect(X,A4982::STEP_IN,	X_STEP_PIN);
		TryConnect(X,A4982::ENABLE_IN,	X_ENABLE_PIN);
		TryConnect(X,A4982::MS1_IN,		X_MS1_PIN);
		TryConnect(X,A4982::MS2_IN,		X_MS2_PIN);
		TryConnect(X,A4982::MIN_OUT,	X_MIN_PIN);
		TryConnect(X,A4982::MAX_OUT,	X_MAX_PIN);

		AddHardware(Y);
		TryConnect(Y,A4982::DIR_IN,		Y_DIR_PIN);
		TryConnect(Y,A4982::STEP_IN,	Y_STEP_PIN);
		TryConnect(Y,A4982::ENABLE_IN,	Y_ENABLE_PIN);
		TryConnect(Y,A4982::MS1_IN,		Y_MS1_PIN);
		TryConnect(Y,A4982::MS2_IN,		Y_MS2_PIN);
		TryConnect(Y,A4982::MIN_OUT,	Y_MIN_PIN);
		TryConnect(Y,A4982::MAX_OUT,	Y_MAX_PIN);

		AddHardware(Z);
		TryConnect(Z,A4982::DIR_IN,		Z_DIR_PIN);
		TryConnect(Z,A4982::STEP_IN,	Z_STEP_PIN);
		TryConnect(Z,A4982::ENABLE_IN,	Z_ENABLE_PIN);
		TryConnect(Z,A4982::MS1_IN,		Z_MS1_PIN);
		TryConnect(Z,A4982::MS2_IN,		Z_MS2_PIN);
		TryConnect(Z,A4982::MIN_OUT,	Z_MIN_PIN);
		TryConnect(Z,A4982::MAX_OUT,	Z_MAX_PIN);

		E.GetConfig().bHasNoEndStops = true;
		AddHardware(E);
		TryConnect(E,A4982::DIR_IN,		E0_DIR_PIN);
		TryConnect(E,A4982::STEP_IN,	E0_STEP_PIN);
		TryConnect(E,A4982::ENABLE_IN,	E0_ENABLE_PIN);
		TryConnect(E,A4982::MS1_IN,		E0_MS1_PIN);
		TryConnect(E,A4982::MS2_IN,		E0_MS2_PIN);

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
