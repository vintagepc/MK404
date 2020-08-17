/*
	EinsyRambo.cpp - Board definition for the Prusa EinsyRambo
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

#include "Test_Board.h"
#include "Test_Wiring.h"        // for Einsy_1_1a
#include "RotaryEncoder.h"           // for HD44780
#include "PinNames.h"          // for Pin, Pin::BTN_ENC, Pin::W25X20CL_PIN_CS


namespace Boards
{
	void Test_Board::SetupHardware()
	{
		DisableInterruptLevelPoll(8);

		AddHardware(m_Monitor,'0');

		AddHardware(encoder);
		TryConnect(encoder, RotaryEncoder::OUT_A, BTN_EN2);
		TryConnect(encoder, RotaryEncoder::OUT_B, BTN_EN1);
		TryConnect(encoder, RotaryEncoder::OUT_BUTTON, BTN_ENC);

		AddHardware(m_btn);
		TryConnect(m_btn, Button::BUTTON_OUT, BTN_ENC);

		AddHardware(m_IR,0);
		TryConnect(m_IR,IRSensor::DIGITAL_OUT, IR_SENSOR_PIN);

		AddHardware(m_spiFlash, GetDIRQ(W25X20CL_PIN_CS));

	}

	// Convenience function for debug printing a particular pin.
	void Test_Board::DebugPin(avr_irq_t *irq, uint32_t value)
	{
		printf("Pin DBG: change to %8x\n",value);
	}

	void Test_Board::OnAVRInit()
	{
		std::string strSPI = GetStorageFileName("xflash");
		m_spiFlash.Load(strSPI.c_str());
	}

	void Test_Board::OnAVRDeinit()
	{
		m_spiFlash.Save();
	}

	void Test_Board::OnAVRReset()
	{

	}

};
