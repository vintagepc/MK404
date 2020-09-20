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
#include "3rdParty/MK3/thermistortables.h"  // for OVERSAMPLENR, temptable_1, temptable_2000
#include "Beeper.h"
#include "HD44780.h"           // for HD44780
#include "LED.h"
#include "PinNames.h"          // for Pin, Pin::BTN_ENC, Pin::W25X20CL_PIN_CS
#include <iostream>
#include <string>
#include <vector>

namespace Boards
{
	void MiniRambo::SetupHardware()
	{
		DisableInterruptLevelPoll(8);

		AddHardware(UART0);

		AddHardware(m_Mon0,'0');

		AddHardware(lcd);
		// D4-D7,
		std::vector<PinNames::Pin> vePins = {LCD_PINS_D4,LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7};
		for (int i = 0; i < 4; i++) {
			TryConnect(vePins.at(i),lcd, HD44780::D4+i);
			TryConnect(lcd, HD44780::D4+i,vePins.at(i));
		}
		TryConnect(LCD_PINS_RS,lcd, HD44780::RS);
		TryConnect(LCD_PINS_ENABLE, lcd,HD44780::E);

		AddHardware(encoder);
		TryConnect(encoder, RotaryEncoder::OUT_A, BTN_EN2);
		TryConnect(encoder, RotaryEncoder::OUT_B, BTN_EN1);
		TryConnect(encoder, RotaryEncoder::OUT_BUTTON, BTN_ENC);

		AddHardware(tExtruder,GetPinNumber(TEMP_0_PIN));
		//NOLINTNEXTLINE - so we can keep using thermistortables.h as-is.
		tExtruder.SetTable({(int16_t*)temptable_5, sizeof(temptable_5)/sizeof(int16_t)}, OVERSAMPLENR);

		AddHardware(tBed,GetPinNumber(TEMP_BED_PIN));
		//NOLINTNEXTLINE - so we can keep using thermistortables.h as-is.
		tBed.SetTable({(int16_t*)temptable_1, sizeof(temptable_1)/sizeof(int16_t)},OVERSAMPLENR);

		AddHardware(tAmbient,  GetPinNumber(TEMP_AMBIENT_PIN));
		//NOLINTNEXTLINE - so we can keep using thermistortables.h as-is.
		tAmbient.SetTable({(int16_t*)temptable_2000, sizeof(temptable_2000)/sizeof(int16_t)}, OVERSAMPLENR);

		AddHardware(fExtruder, 	nullptr, GetDIRQ(E0_FAN), GetPWMIRQ(E0_FAN));
		AddHardware(fPrint, 	nullptr, GetDIRQ(FAN_PIN), GetPWMIRQ(FAN_PIN));

		AddHardware(hBed, nullptr, GetDIRQ(HEATER_BED_PIN));
		hBed.ConnectTo(Heater::TEMP_OUT, tBed.GetIRQ(Thermistor::TEMP_IN));

		AddHardware(hExtruder, nullptr, GetDIRQ(HEATER_0_PIN));
		hExtruder.ConnectTo(Heater::TEMP_OUT, tExtruder.GetIRQ(Thermistor::TEMP_IN));

		AddHardware(m_buzzer);
		m_buzzer.ConnectFrom(GetDIRQ(BEEPER),Beeper::DIGITAL_IN);

		X.GetConfig().bInverted = true;
		X.GetConfig().iMaxMM = 255;
		AddHardware(X);
		TryConnect(X_DIR_PIN, 		X, A4982::DIR_IN);
		TryConnect(X_STEP_PIN, 		X, A4982::STEP_IN);
		TryConnect(X_ENABLE_PIN, 	X, A4982::ENABLE_IN);
		TryConnect(X_MS1_PIN, 		X, A4982::MS1_IN);
		TryConnect(X_MS2_PIN, 		X, A4982::MS2_IN);
		TryConnect(X,A4982::MIN_OUT,	X_MIN_PIN);
		//TryConnect(X,A4982::MAX_OUT,	X_MAX_PIN);

		Y.GetConfig().bInverted = true;
		Y.GetConfig().iMaxMM = 210;
		AddHardware(Y);
		TryConnect(Y_DIR_PIN, 		Y, A4982::DIR_IN);
		TryConnect(Y_STEP_PIN, 		Y, A4982::STEP_IN);
		TryConnect(Y_ENABLE_PIN, 	Y, A4982::ENABLE_IN);
		TryConnect(Y_MS1_PIN, 		Y, A4982::MS1_IN);
		TryConnect(Y_MS2_PIN, 		Y, A4982::MS2_IN);
		TryConnect(Y,A4982::MIN_OUT,	Y_MIN_PIN);
		//TryConnect(Y,A4982::MAX_OUT,	Y_MAX_PIN);

		Z.GetConfig().bInverted = true;
		Z.GetConfig().uiStepsPerMM = 400;
		Z.GetConfig().iMaxMM = 210;
		Z.GetConfig().fStartPos = 10.f;
		AddHardware(Z);
		TryConnect(Z_DIR_PIN, 		Z, A4982::DIR_IN);
		TryConnect(Z_STEP_PIN, 		Z, A4982::STEP_IN);
		TryConnect(Z_ENABLE_PIN, 	Z, A4982::ENABLE_IN);
		TryConnect(Z_MS1_PIN, 		Z, A4982::MS1_IN);
		TryConnect(Z_MS2_PIN, 		Z, A4982::MS2_IN);
		//TryConnect(Z,A4982::MIN_OUT,	Z_MIN_PIN);
		//TryConnect(Z,A4982::MAX_OUT,	Z_MAX_PIN);

		E.GetConfig().bHasNoEndStops = true;
		AddHardware(E);
		TryConnect(E0_DIR_PIN, 		E, A4982::DIR_IN);
		TryConnect(E0_STEP_PIN,		E, A4982::STEP_IN);
		TryConnect(E0_ENABLE_PIN, 	E, A4982::ENABLE_IN);
		TryConnect(E0_MS1_PIN, 		E, A4982::MS1_IN);
		TryConnect(E0_MS2_PIN, 		E, A4982::MS2_IN);

		AddUARTTrace('0'); // External

		// SD card
		std::string strSD = GetSDCardFile();
		sd_card.SetImage(strSD);
		AddHardware(sd_card);
		TryConnect(PinNames::Pin::SDSS, sd_card, SDCard::SPI_CSEL);

		// wire up the SD present signal.
		TryConnect(sd_card, SDCard::CARD_PRESENT, SDCARDDETECT);

		// Add indicator first so it captures the mount IRQ
		AddHardware(lSD);
		TryConnect(SDCARDDETECT, lSD, LED::LED_IN);

		int mount_error = sd_card.Mount();

		if (mount_error != 0) {
			std::cerr << "SD card image (" << strSD << ") could not be mounted (error " << mount_error << " ).\n";
		}

	}

	// pragma: LCOV_EXCL_START
	// Convenience function for debug printing a particular pin.
	void MiniRambo::DebugPin(avr_irq_t */*irq*/, uint32_t value)
	{
		std::cout << "Pin DBG: change to " << std::hex << value << '\n';
	}
	// pragma: LCOV_EXCL_STOP

	void MiniRambo::OnAVRInit()
	{
	}

	void MiniRambo::OnAVRDeinit()
	{
		sd_card.Unmount();
	}

	void MiniRambo::OnAVRReset()
	{
		std::cout << "RESET\n";
		DisableInterruptLevelPoll(8);

		SetPin(BTN_ENC,1);

		UART0.Reset();
	}

}; // namespace Boards
