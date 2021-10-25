/*
	CW1S.h - Board definition for the Prusa CW1/S
	Copyright 2021 VintagePC <https://github.com/vintagepc/>

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

#include "CW1S.h"
#include "3rdParty/MK3/thermistortables_2.h"
#include "PinNames.h"  // for Pin::FINDA_PIN, Pin::I_TMC2130_DIAG, Pin::P_TM...
#include <GL/freeglut_std.h>          // for glutGet, glutStrokeCharacter, GLUT_STRO...
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glTranslatef, glVertex3f, glColor3f
#else
# include <GL/gl.h>           // for glTranslatef, glVertex3f, glColor3f
#endif
#include "gsl-lite.hpp"

static constexpr char strCW1[17] = "01_4040404040404";
static constexpr char strCW1S[17] = "02_4040404040404";

namespace Boards
{

	CW1S::~CW1S()
	{
		#ifndef __APPLE__
			pthread_cancel(m_usb_thread);
			usbip_destroy(m_usb);
		#endif
	}

	void CW1S::SetupHardware()
	{
		gsl::span<uint8_t> flash {m_pAVR->flash, m_pAVR->flashend};

		if (m_bIsCW1S)
		{
			std::memcpy(&flash[0x7FE0], &strCW1S, 16);
		}
		else
		{
			std::memcpy(&flash[0x7FE0], &strCW1, 16);
		}

		DisableInterruptLevelPoll(5);
		AddHardware(m_lcd);
		// D4-D7,
		std::vector<PinNames::Pin> vePins = {LCD_PINS_D4,LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7};
		for (int i = 0; i < 4; i++) {
			TryConnect(vePins.at(i),&m_lcd, HD44780::D4+i);
			TryConnect(&m_lcd, HD44780::D4+i,vePins.at(i));
		}
		TryConnect(LCD_PINS_RS,&m_lcd, HD44780::RS);
		TryConnect(LCD_PINS_ENABLE, &m_lcd,HD44780::E);
		m_lcd.ConnectFrom(GetPWMIRQ(LCD_BL_PIN), HD44780::BRIGHTNESS_PWM_IN);

		AddHardware(m_gpio);
		TryConnect(MCP_CSEL,&m_gpio, MCP23S17::SPI_CSEL);

		AddHardware(m_enc);
		TryConnect(&m_enc, RotaryEncoder::OUT_A, BTN_EN2);
		TryConnect(&m_enc, RotaryEncoder::OUT_B, BTN_EN1);
		avr_connect_irq(m_enc.GetIRQ(RotaryEncoder::OUT_BUTTON), m_gpio.GetIRQ(MCP23S17::MCP_GPA0));

		AddHardware(m_beep);
		TryConnect(BEEPER, &m_beep, Beeper::DIGITAL_IN);

		AddHardware(m_tmc);
		auto cfg = m_tmc.GetConfig();
		cfg.bHasNoEndStops = true;
		cfg.uiFullStepsPerMM = 200;
		m_tmc.SetConfig(cfg);
		m_tmc.SetSimple(true);
		TryConnect(X_STEP_PIN, &m_tmc, TMC2130::STEP_IN);
		TryConnect(X_TMC2130_CS, &m_tmc, TMC2130::SPI_CSEL);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA3), m_tmc.GetIRQ(TMC2130::ENABLE_IN));
		avr_connect_irq(m_tmc.GetIRQ(TMC2130::DIAG_OUT),m_gpio.GetIRQ(MCP23S17::MCP_GPA4));
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA5), m_tmc.GetIRQ(TMC2130::DIR_IN));

		m_lid.SetIsToggle(true);
		AddHardware(m_lid);
		m_lid.Press();
		avr_connect_irq(m_lid.GetIRQ(Button::BUTTON_OUT), m_gpio.GetIRQ(MCP23S17::MCP_GPA2));

		m_tank.SetIsToggle(true);
		AddHardware(m_tank);
		m_tank.Press();
		avr_connect_irq(m_tank.GetIRQ(Button::BUTTON_OUT), m_gpio.GetIRQ(MCP23S17::MCP_GPA1));

		GetPWMIRQ(FAN_PIN)->flags |= IRQ_FLAG_PWM_INV;
		GetPWMIRQ(FAN_1_PIN)->flags |= IRQ_FLAG_PWM_INV;
		AddHardware(m_f1,GetDIRQ(TACH_0),  GetDIRQ(FAN_PIN), GetPWMIRQ(FAN_PIN), true);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPB3), m_f1.GetIRQ(Fan::ENABLE_IN));
		AddHardware(m_f2, GetDIRQ(TACH_1), GetDIRQ(FAN_1_PIN), GetPWMIRQ(FAN_1_PIN), true);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPB4), m_f2.GetIRQ(Fan::ENABLE_IN));
		if (!m_bIsCW1S)
		{
			AddHardware(m_f3,GetDIRQ(TACH_2), m_gpio.GetIRQ(MCP23S17::MCP_GPB2), nullptr);
		}

		AddHardware(m_ht, nullptr, m_gpio.GetIRQ(MCP23S17::MCP_GPB2));

		AddHardware(m_htUV, GetPWMIRQ(LED_PIN), m_gpio.GetIRQ(MCP23S17::MCP_GPA6));

		AddHardware(m_muxY,1);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPB1), m_muxY.GetIRQ(L74HCT4052::A_IN));
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPB0), m_muxY.GetIRQ(L74HCT4052::B_IN));


		AddHardware(m_uv);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA6), m_uv.GetIRQ(LED::LED_IN));
		avr_connect_irq(GetPWMIRQ(LED_PIN), m_uv.GetIRQ(LED::PWM_IN));

		AddHardware(m_tUV,0xFF);
		//NOLINTNEXTLINE - so we can keep using thermistortables.h as-is.
		m_tUV.SetTable({(int16_t*)table_NCP21WF104J03RA,
			sizeof(table_NCP21WF104J03RA) / sizeof(int16_t)},
			1);
		avr_connect_irq(m_muxY.GetIRQ(L74HCT4052::OUT_1),m_tUV.GetIRQ(Thermistor::ADC_TRIGGER_IN));
		avr_connect_irq(m_tUV.GetIRQ(Thermistor::ADC_VALUE_OUT),m_muxY.GetIRQ(L74HCT4052::IN_1));

		AddHardware(m_tAmb,0xFF);
		//NOLINTNEXTLINE - so we can keep using thermistortables.h as-is.
		m_tAmb.SetTable({(int16_t*)table_100k_ntc,
			sizeof(table_100k_ntc) / sizeof(int16_t)},
			1);
		avr_connect_irq(m_muxY.GetIRQ(L74HCT4052::OUT_0),m_tAmb.GetIRQ(Thermistor::ADC_TRIGGER_IN));
		avr_connect_irq(m_tAmb.GetIRQ(Thermistor::ADC_VALUE_OUT),m_muxY.GetIRQ(L74HCT4052::IN_0));

		m_ht.ConnectTo(Heater::TEMP_OUT, m_tAmb.GetIRQ(Thermistor::TEMP_IN));
		m_htUV.ConnectTo(Heater::TEMP_OUT, m_tUV.GetIRQ(Thermistor::TEMP_IN));

		#ifndef __APPLE__ // pragma: LCOV_EXCL_START
			m_usb = usbip_create(m_pAVR);
			if (!m_usb)
			{
				std::cout << "Could not create USBIP context. Skipping thread...\n";
			}
			else
			{
				pthread_create(&m_usb_thread, nullptr, usbip_main, m_usb);
			}
		#endif // pragma: LCOV_EXCL_STOP
	}

	void CW1S::Draw()		/* function called whenever redisplay needed */
	{
		glPushMatrix();
		glLoadIdentity(); // Start with an identity matrix
			glScalef(4, 4, 1);
			m_lcd.Draw();
		glPopMatrix();
		// Do something for the motors...
		float fX = (5 + m_lcd.GetWidth()* 6)*4;
		float fY = (5 + m_lcd.GetHeight() * 9);
		glLoadIdentity();
		glScalef(fX/350,4,1);
		glPushMatrix();
			glTranslatef(0, fY,0);
			m_tmc.Draw();
			glTranslatef(170,0,0);
			m_beep.Draw();
			glTranslatef(20,0,0);
			m_f1.Draw();
			glTranslatef(20,0,0);
			m_f2.Draw();
			glTranslatef(20,0,0);
			m_f3.Draw();
			glTranslatef(20,0,0);
			m_ht.Draw();
			glTranslatef(20,0,0);
			m_htUV.Draw();
			glTranslatef(20,0,0);
			m_uv.Draw();
		glPopMatrix();
	}

}; // namespace Boards
