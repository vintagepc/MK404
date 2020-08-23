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
#include "thermistortables.h"


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

		AddHardware(m_vSrc,1);
		TryConnect(m_vSrc,VoltageSrc::DIGITAL_OUT, VOLT_PWR_PIN);

		AddHardware(m_btns,2);

		m_thrm.SetTable((short*)temptable_5,
								sizeof(temptable_5) / sizeof(short) / 2,
								OVERSAMPLENR);

		AddHardware(m_thrm,3);


		TMC2130::TMC2130_cfg_t cfg;
		cfg.iMaxMM = 20;
		cfg.uiStepsPerMM=1;

		m_TMC.SetConfig(cfg);
		AddHardware(m_TMC);

		TryConnect(X_TMC2130_CS, 	m_TMC, TMC2130::SPI_CSEL);
		TryConnect(X_DIR_PIN,		m_TMC, TMC2130::DIR_IN);
		TryConnect(X_STEP_PIN,		m_TMC, TMC2130::STEP_IN);
		TryConnect(X_ENABLE_PIN,	m_TMC, TMC2130::ENABLE_IN);
		TryConnect(m_TMC,TMC2130::DIAG_OUT, X_TMC2130_DIAG);

		AddHardware(m_shift);
		TryConnect(SHIFT_LATCH,		m_shift, HC595::IN_LATCH);
		TryConnect(SHIFT_CLOCK,		m_shift, HC595::IN_CLOCK);
		TryConnect(SHIFT_DATA,		m_shift, HC595::IN_DATA);
		// Cheat and reuse a line (driver doesn't care, EN is off)
		TryConnect(X_DIR_PIN, m_shift, HC595::IN_RESET);

		AddHardware(m_heat,GetPWMIRQ(HEATER_0_PIN), GetDIRQ(HEATER_0_PIN));

		AddHardware(m_Fan,GetDIRQ(TACH_0), GetDIRQ(FAN_PIN), GetPWMIRQ(FAN_PIN));

		AddHardware(m_Sniff,'0');

		AddHardware(m_pinda, nullptr, nullptr, nullptr);
		TryConnect(m_pinda, PINDA::TRIGGER_OUT, Z_MIN_PIN);

		AddHardware(m_lcd);
		PinNames::Pin ePins[4] = {LCD_PINS_D4,LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7};
		for (int i = 0; i < 4; i++) {
			TryConnect(ePins[i],m_lcd, HD44780::D4+i);
		}
		TryConnect(LCD_PINS_RS,m_lcd, HD44780::RS);
		TryConnect(LCD_PINS_ENABLE, m_lcd,HD44780::E);

		// SD card
		string strSD = GetSDCardFile();
		m_card.SetImage(strSD);
		AddHardware(m_card);
		TryConnect(SDSS,m_card, SDCard::SPI_CSEL);
		TryConnect(m_card, SDCard::CARD_PRESENT, SDCARDDETECT);

		m_card.Mount();

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
