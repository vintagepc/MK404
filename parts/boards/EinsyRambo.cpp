/*
	EinsyRambo.cpp - Board definition for the Prusa EinsyRambo
	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK3SIM.

	MK3SIM is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK3SIM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK3SIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boards/EinsyRambo.h>
#include <thermistortables.h>

#define TEMP_SENSOR_0 5
#define TEMP_SENSOR_BED 1
#define TEMP_SENSOR_AMBIENT 2000

#define _TERMISTOR_TABLE(num) \
		temptable_##num
#define TERMISTOR_TABLE(num) \
		_TERMISTOR_TABLE(num)


namespace Boards
{
	void EinsyRambo::SetupHardware()
	{
		DisableInterruptLevelPoll(8);

		AddSerialPty(UART2,'2');
		AddHardware(UART0);

		// SD card
		sd_card_init (m_pAVR, &sd_card);
		sd_card_attach (m_pAVR, &sd_card, AVR_IOCTL_SPI_GETIRQ (0), m_wiring.DIRQLU(m_pAVR,SDSS));

		// wire up the SD present signal.
		TryConnect(sd_card.irq + IRQ_SD_CARD_PRESENT, SDCARDDETECT);

		// Add indicator first so it captures the mount IRQ
		AddHardware(lSD);
		TryConnect(SDCARDDETECT, lSD, LED::LED_IN);

		std::string strSD = GetStorageFileName("SDcard");
		snprintf(sd_card.filepath, sizeof(sd_card.filepath), strSD.c_str());
		int mount_error = sd_card_mount_file (m_pAVR, &sd_card, sd_card.filepath, 0);

		if (mount_error != 0) {
			fprintf (stderr, "SD card image ‘%s’ could not be mounted (error %i).\n", sd_card.filepath, mount_error);
		}

		// Heaters
		AddHardware(tExtruder,GetPinNumber(TEMP_0_PIN));
		tExtruder.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_0),
								sizeof(TERMISTOR_TABLE(TEMP_SENSOR_0)) / sizeof(short) / 2,
								OVERSAMPLENR);

		AddHardware(tBed,GetPinNumber(TEMP_BED_PIN));
		tBed.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_BED),
							sizeof(TERMISTOR_TABLE(TEMP_SENSOR_BED)) / sizeof(short) / 2,
							OVERSAMPLENR);

		// same table as bed.
		AddHardware(tPinda, GetPinNumber(TEMP_PINDA_PIN));
		tPinda.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_BED),
							sizeof(TERMISTOR_TABLE(TEMP_SENSOR_BED)) / sizeof(short) / 2,
							OVERSAMPLENR);

		AddHardware(tAmbient,  GetPinNumber(TEMP_AMBIENT_PIN));
		tAmbient.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT),
		 						sizeof(TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT)) / sizeof(short) / 2,
		 						OVERSAMPLENR);

		AddHardware(fExtruder, GetDIRQ(TACH_0), GetDIRQ(E0_FAN), GetPWMIRQ(E0_FAN));
		AddHardware(fPrint, GetDIRQ(TACH_1), GetDIRQ(FAN_PIN), GetPWMIRQ(FAN_PIN));

		AddHardware(hBed, nullptr, GetDIRQ(HEATER_BED_PIN));
		hBed.ConnectTo(Heater::TEMP_OUT, tBed.GetIRQ(Thermistor::TEMP_IN));

		AddHardware(hExtruder, nullptr, GetDIRQ(HEATER_0_PIN));
		hExtruder.ConnectTo(Heater::TEMP_OUT, tExtruder.GetIRQ(Thermistor::TEMP_IN));

		AddHardware(lcd);
		// D4-D7,
		PinNames::Pin ePins[4] = {LCD_PINS_D4,LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7};
		for (int i = 0; i < 4; i++) {
			TryConnect(ePins[i],lcd, HD44780::D4+i);
			TryConnect(lcd, HD44780::D4+i,ePins[i]);
		}
		TryConnect(LCD_PINS_RS,lcd, HD44780::RS);
		TryConnect(LCD_PINS_ENABLE, lcd,HD44780::E);
		TryConnect(LCD_BL_PIN, lcd, HD44780::BRIGHTNESS_IN);
		TryConnect(LCD_BL_PIN, lcd, HD44780::BRIGHTNESS_PWM_IN);

		AddHardware(encoder);
		TryConnect(encoder, RotaryEncoder::OUT_A, BTN_EN2);
		TryConnect(encoder, RotaryEncoder::OUT_B, BTN_EN1);
		TryConnect(encoder, RotaryEncoder::OUT_BUTTON, BTN_ENC);

		TMC2130::TMC2130_cfg_t cfg;
		cfg.iMaxMM = 255;
		cfg.cAxis = 'X';

		X.SetConfig(cfg);
		AddHardware(X);
		TryConnect(X_TMC2130_CS, X, TMC2130::SPI_CSEL);
		TryConnect(X_DIR_PIN,	X, TMC2130::DIR_IN);
		TryConnect(X_STEP_PIN,	X, TMC2130::STEP_IN);
		TryConnect(X_ENABLE_PIN,	X, TMC2130::ENABLE_IN);
		TryConnect(X,TMC2130::DIAG_OUT, X_TMC2130_DIAG);

		cfg.uiStepsPerMM = 400;
		cfg.iMaxMM = 219;
		cfg.cAxis = 'Z';

		Z.SetConfig(cfg);
		AddHardware(Z);
		TryConnect(Z_TMC2130_CS , 	Z, TMC2130::SPI_CSEL);
		TryConnect(Z_DIR_PIN ,		Z, TMC2130::DIR_IN);
		TryConnect(Z_STEP_PIN ,		Z, TMC2130::STEP_IN);
		TryConnect(Z_ENABLE_PIN ,	Z, TMC2130::ENABLE_IN);
		TryConnect(Z, TMC2130::DIAG_OUT, Z_TMC2130_DIAG);

		cfg.bInverted = true;
		cfg.uiStepsPerMM = 100;
		cfg.cAxis = 'Y';
		cfg.iMaxMM = 220;

		Y.SetConfig(cfg);
		AddHardware(Y);
		TryConnect(Y_TMC2130_CS, 	Y,TMC2130::SPI_CSEL);
		TryConnect(Y_DIR_PIN,		Y,TMC2130::DIR_IN);
		TryConnect(Y_STEP_PIN,		Y,TMC2130::STEP_IN);
		TryConnect(Y_ENABLE_PIN,		Y,TMC2130::ENABLE_IN);
		TryConnect(Y,TMC2130::DIAG_OUT,Y_TMC2130_DIAG);

		cfg.bHasNoEndStops = true;
		cfg.fStartPos = 0;
		cfg.uiStepsPerMM = 280;
		cfg.cAxis = 'E';

		E.SetConfig(cfg);
		AddHardware(E);
		TryConnect(E0_TMC2130_CS, 	E, TMC2130::SPI_CSEL);
		TryConnect(E0_DIR_PIN,		E, TMC2130::DIR_IN);
		TryConnect(E0_STEP_PIN,		E, TMC2130::STEP_IN);
		TryConnect(E0_ENABLE_PIN,	E, TMC2130::ENABLE_IN);
		TryConnect(E, TMC2130::DIAG_OUT, E0_TMC2130_DIAG);

		AddHardware(pinda, X.GetIRQ(TMC2130::POSITION_OUT),  Y.GetIRQ(TMC2130::POSITION_OUT),  Z.GetIRQ(TMC2130::POSITION_OUT));
		TryConnect(pinda, PINDA::TRIGGER_OUT ,Z_MIN_PIN);
		AddHardware(lPINDA);
		lPINDA.ConnectFrom(pinda.GetIRQ(PINDA::TRIGGER_OUT), LED::LED_IN);

		AddHardware(vBed, GetPinNumber(VOLT_BED_PIN));
		AddHardware(vMain, GetPinNumber(VOLT_PWR_PIN));
		AddHardware(IR, GetPinNumber(VOLT_IR_PIN));
		TryConnect(IR,IRSensor::DIGITAL_OUT, IR_SENSOR_PIN);
		AddHardware(lIR);
		TryConnect(IR_SENSOR_PIN, lIR, LED::LED_IN);

		AddHardware(PowerPanic);
		TryConnect(PowerPanic, Button::BUTTON_OUT, UVLO_PIN); // Note - PP is not defined in pins_einsy, it's an EXTINT.

	}

	void EinsyRambo::OnAVRInit()
	{
		std::string strSPI = GetStorageFileName("xflash");
		spiFlash.Load(strSPI.c_str());
	}

	void EinsyRambo::OnAVRDeinit()
	{
		spiFlash.Save();
		sd_card_unmount_file(m_pAVR, &sd_card);
	}

	void EinsyRambo::OnAVRReset()
	{
		printf("RESET\n");
		DisableInterruptLevelPoll(8);

		// Restore powerpanic to high
		PowerPanic.Press(1);

		UART0.Reset();

		//depress encoder knob
		if (!m_bFactoryReset)
			SetPin(BTN_ENC,1);
		else
			SetPin(BTN_ENC,0);

		m_bFactoryReset = false;

		// TIMSK2
		avr_regbit_t rb = AVR_IO_REGBITS(0x70, 0, 0b111);
		avr_regbit_setto(m_pAVR,rb,0x01);

		//Reset all SPI SS lines
		SetPin(W25X20CL_PIN_CS,1);
		SetPin(SDSS,1);
		SetPin(X_TMC2130_CS,1);
		SetPin(Y_TMC2130_CS,1);
		SetPin(Z_TMC2130_CS,1);
		SetPin(E0_TMC2130_CS,1);
	}



};
