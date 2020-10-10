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
#include "3rdParty/MK3/thermistortables.h"
#include "HD44780.h"
#include "MMU1.h"
#include "PinNames.h"          // for Pin, Pin::BTN_ENC, Pin::W25X20CL_PIN_CS
#include "RotaryEncoder.h"           // for HD44780

#include <iostream>
#include <string>
#include <vector>


namespace Boards
{
	void Test_Board::SetupHardware()
	{
		DisableInterruptLevelPoll(8);

		AddHardware(m_Monitor,'0');

		AddHardware(encoder);
		TryConnect(&encoder, RotaryEncoder::OUT_A, BTN_EN2);
		TryConnect(&encoder, RotaryEncoder::OUT_B, BTN_EN1);
		TryConnect(&encoder, RotaryEncoder::OUT_BUTTON, BTN_ENC);

		AddHardware(m_btn);
		TryConnect(&m_btn, Button::BUTTON_OUT, BTN_ENC);

		AddHardware(m_IR,0);
		TryConnect(&m_IR,IRSensor::DIGITAL_OUT, IR_SENSOR_PIN);

		AddHardware(m_spiFlash, GetDIRQ(W25X20CL_PIN_CS));

		AddHardware(m_vSrc,1);
		TryConnect(&m_vSrc,VoltageSrc::DIGITAL_OUT, VOLT_PWR_PIN);

		AddHardware(m_btns,2);
		//NOLINTNEXTLINE - so we can keep using thermistortables.h as-is.
		m_thrm.SetTable({(int16_t*)temptable_5, sizeof(temptable_5)/sizeof(int16_t)},OVERSAMPLENR);

		AddHardware(m_thrm,3);


		TMC2130::TMC2130_cfg_t cfg;
		cfg.iMaxMM = 20;
		cfg.uiFullStepsPerMM=16;

		m_TMC.SetConfig(cfg);
		AddHardware(m_TMC);

		TryConnect(X_TMC2130_CS, 	&m_TMC, TMC2130::SPI_CSEL);
		TryConnect(X_DIR_PIN,		&m_TMC, TMC2130::DIR_IN);
		TryConnect(X_STEP_PIN,		&m_TMC, TMC2130::STEP_IN);
		TryConnect(X_ENABLE_PIN,	&m_TMC, TMC2130::ENABLE_IN);
		TryConnect(&m_TMC,TMC2130::DIAG_OUT, X_TMC2130_DIAG);

		AddHardware(m_shift);
		TryConnect(SHIFT_LATCH,		&m_shift, HC595::IN_LATCH);
		TryConnect(SHIFT_CLOCK,		&m_shift, HC595::IN_CLOCK);
		TryConnect(SHIFT_DATA,		&m_shift, HC595::IN_DATA);
		// Cheat and reuse a line (driver doesn't care, EN is off)
		TryConnect(X_DIR_PIN, &m_shift, HC595::IN_RESET);





		AddHardware(m_heat,GetPWMIRQ(HEATER_0_PIN), GetDIRQ(HEATER_0_PIN));

		AddHardware(m_Fan,GetDIRQ(TACH_0), GetDIRQ(FAN_PIN), GetPWMIRQ(FAN_PIN));

		AddHardware(m_Sniff,'0');

		AddHardware(m_pinda, nullptr, nullptr, nullptr);
		TryConnect(&m_pinda, PINDA::TRIGGER_OUT, Z_MIN_PIN);

		AddHardware(m_lcd);
		std::vector<PinNames::Pin> vePins = {LCD_PINS_D4,LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7};
		for (int i = 0; i < 4; i++) {
			TryConnect(vePins.at(i),&m_lcd, HD44780::D4+i);
		}
		TryConnect(LCD_PINS_RS,&m_lcd, HD44780::RS);
		TryConnect(LCD_PINS_ENABLE, &m_lcd,HD44780::E);

		// SD card
		std::string strSD = GetSDCardFile();
		m_card.SetImage(strSD);
		AddHardware(m_card);
		TryConnect(SDSS, &m_card, SDCard::SPI_CSEL);
		TryConnect(&m_card, SDCard::CARD_PRESENT, SDCARDDETECT);

		AddHardware(m_pat, GetDIRQ(SWI2C_SCL), GetDIRQ(SWI2C_SDA));

		AddHardware(m_LED);
		AddHardware(m_LED2);
		TryConnect(LCD_BL_PIN, &m_LED, LED::LED_IN);
		m_LED.ConnectFrom(GetPWMIRQ(LCD_BL_PIN), LED::PWM_IN);
		TryConnect(LCD_BL_PIN, &m_LED2, LED::LED_IN);
		m_LED2.ConnectFrom(GetPWMIRQ(LCD_BL_PIN), LED::PWM_IN);
		TryConnect(LCD_BL_PIN, &m_lcd, HD44780::BRIGHTNESS_IN);
		m_lcd.ConnectFrom(GetPWMIRQ(LCD_BL_PIN), HD44780::BRIGHTNESS_PWM_IN);

		AddHardware(m_buzzer);
		m_buzzer.ConnectFrom(GetDIRQ(BEEPER), Beeper::DIGITAL_IN);

		AddHardware(m_MM1);
		TryConnect(E_MUX0_PIN, &m_MM1, MMU1::MUX0);
		TryConnect(E_MUX1_PIN, &m_MM1, MMU1::MUX1);
		TryConnect(X_STEP_PIN, &m_MM1, MMU1::STEP_IN);


		m_Allg.GetConfig().iMaxMM = 20;
		m_Allg.GetConfig().fStartPos = 10.f;
		m_Allg.GetConfig().uiStepsPerMM = 10*16; // Needs to be at the full step count for 16ms, regardless of actual MS setting.
		AddHardware(m_Allg);
		TryConnect(X_DIR_PIN, &m_Allg, A4982::DIR_IN);
		TryConnect(X_STEP_PIN, &m_Allg, A4982::STEP_IN);
		TryConnect(X_ENABLE_PIN, &m_Allg, A4982::ENABLE_IN);
		TryConnect(E_MUX0_PIN, &m_Allg, A4982::MS1_IN);
		TryConnect(E_MUX1_PIN, &m_Allg, A4982::MS2_IN);
		TryConnect(X_RST_PIN, &m_Allg, A4982::RESET_IN);
		TryConnect(X_SLP_PIN, &m_Allg, A4982::SLEEP_IN);
		TryConnect(&m_Allg, A4982::MAX_OUT, X_MAX_PIN);
		TryConnect(&m_Allg, A4982::MIN_OUT, X_MAX_PIN);

	}

	// Convenience function for debug printing a particular pin.
	void Test_Board::DebugPin(avr_irq_t *, uint32_t value) // pragma: LCOV_EXCL_START
	{
		std::cout << "Pin DBG: change to " << std::hex << value << '\n';
	}

	// pragma: LCOV_EXCL_STOP

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

}; // namespace Boards
