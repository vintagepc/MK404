/*
	SL1.cpp - Board definition for the Prusa SL1 control board
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

#include "SL1.h"
#include "PinNames.h"  // for Pin::FINDA_PIN, Pin::I_TMC2130_DIAG, Pin::P_TM...
#include <GL/freeglut_std.h>          // for glutGet, glutStrokeCharacter, GLUT_STRO...
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glTranslatef, glVertex3f, glColor3f
#else
# include <GL/gl.h>           // for glTranslatef, glVertex3f, glColor3f
#endif
#include "gsl-lite.hpp"

namespace Boards
{

	SL1::~SL1()
	{
		#ifndef __APPLE__
			pthread_cancel(m_usb_thread);
			usbip_destroy(m_usb);
		#endif
	}

	void SL1::SetupHardware()
	{
		DisableInterruptLevelPoll(5);

		AddSerialPty(&m_uart,'1');

		AddHardware(m_gpio);
		TryConnect(MCP_CSEL,&m_gpio, MCP23S17::SPI_CSEL);

		AddHardware(m_beep);
		TryConnect(BEEPER, &m_beep, Beeper::DIGITAL_IN);

		AddHardware(m_tower);
		auto cfg = m_tower.GetConfig();
		cfg.iMaxMM = 150;
		cfg.uiFullStepsPerMM = 200;
		m_tower.SetConfig(cfg);

		TryConnect(Z_STEP_PIN, &m_tower, TMC2130::STEP_IN);
		TryConnect(Z_TMC2130_CS, &m_tower, TMC2130::SPI_CSEL);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA1), m_tower.GetIRQ(TMC2130::ENABLE_IN));
		TryConnect(m_tower.GetIRQ(TMC2130::DIAG_OUT),Z_TMC2130_DIAG);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA0), m_tower.GetIRQ(TMC2130::DIR_IN));

		AddHardware(m_tilt);
		cfg = m_tilt.GetConfig();
		cfg.iMaxMM = 150;
		cfg.uiFullStepsPerMM = 200;
		m_tilt.SetConfig(cfg);

		TryConnect(Y_STEP_PIN, &m_tilt, TMC2130::STEP_IN);
		TryConnect(Y_TMC2130_CS, &m_tilt, TMC2130::SPI_CSEL);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA3), m_tower.GetIRQ(TMC2130::ENABLE_IN));
		TryConnect(m_tower.GetIRQ(TMC2130::DIAG_OUT),Y_TMC2130_DIAG);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA2), m_tower.GetIRQ(TMC2130::DIR_IN));

		AddHardware(m_pwr);

		//avr_connect_irq(m_lid.GetIRQ(Button::BUTTON_OUT), m_gpio.GetIRQ(MCP23S17::MCP_GPA2));

		//GetPWMIRQ(FAN_PIN)->flags |= IRQ_FLAG_PWM_INV;
		//GetPWMIRQ(FAN_1_PIN)->flags |= IRQ_FLAG_PWM_INV;
		AddHardware(m_f1,GetDIRQ(TACH_0),  nullptr, nullptr);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA4), m_f1.GetIRQ(Fan::DIGITAL_IN));
		AddHardware(m_f2, GetDIRQ(TACH_1), nullptr, nullptr);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA5), m_f2.GetIRQ(Fan::DIGITAL_IN));
		AddHardware(m_f3, GetDIRQ(TACH_2), GetDIRQ(FAN_2_PIN), GetPWMIRQ(FAN_2_PIN), true);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA6), m_f3.GetIRQ(Fan::DIGITAL_IN));


		AddHardware(m_tx);
		TryConnect(TX_LED, &m_tx, LED::LED_IN);

		AddHardware(m_rx);
		TryConnect(RX_LED, &m_rx, LED::LED_IN);

		AddHardware(m_htUV, GetPWMIRQ(LED_PIN), m_gpio.GetIRQ(MCP23S17::MCP_GPA6));

		AddHardware(m_muxY,0);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPB3), m_muxY.GetIRQ(L74HCT4052::A_IN));
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPB4), m_muxY.GetIRQ(L74HCT4052::B_IN));
		// temps in

		AddHardware(m_muxX,1);
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPB3), m_muxX.GetIRQ(L74HCT4052::A_IN));
		avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPB4), m_muxX.GetIRQ(L74HCT4052::B_IN));
		// uv LEDs/DC in

		// AddHardware(m_uv);
		// avr_connect_irq(m_gpio.GetIRQ(MCP23S17::MCP_GPA6), m_uv.GetIRQ(LED::LED_IN));
		// avr_connect_irq(GetPWMIRQ(LED_PIN), m_uv.GetIRQ(LED::PWM_IN));

		// AddHardware(m_tUV,0xFF);
		// //NOLINTNEXTLINE - so we can keep using thermistortables.h as-is.
		// m_tUV.SetTable({(int16_t*)table_NCP21WF104J03RA,
		// 	sizeof(table_NCP21WF104J03RA) / sizeof(int16_t)},
		// 	1);
		// avr_connect_irq(m_muxY.GetIRQ(L74HCT4052::OUT_1),m_tUV.GetIRQ(Thermistor::ADC_TRIGGER_IN));
		// avr_connect_irq(m_tUV.GetIRQ(Thermistor::ADC_VALUE_OUT),m_muxY.GetIRQ(L74HCT4052::IN_1));

		// AddHardware(m_tAmb,0xFF);
		// //NOLINTNEXTLINE - so we can keep using thermistortables.h as-is.
		// m_tAmb.SetTable({(int16_t*)table_100k_ntc,
		// 	sizeof(table_100k_ntc) / sizeof(int16_t)},
		// 	1);
		// avr_connect_irq(m_muxY.GetIRQ(L74HCT4052::OUT_0),m_tAmb.GetIRQ(Thermistor::ADC_TRIGGER_IN));
		// avr_connect_irq(m_tAmb.GetIRQ(Thermistor::ADC_VALUE_OUT),m_muxY.GetIRQ(L74HCT4052::IN_0));

		// m_ht.ConnectTo(Heater::TEMP_OUT, m_tAmb.GetIRQ(Thermistor::TEMP_IN));
		// m_htUV.ConnectTo(Heater::TEMP_OUT, m_tUV.GetIRQ(Thermistor::TEMP_IN));

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

	void SL1::Draw()		/* function called whenever redisplay needed */
	{
		// Do something for the motors...
		float fX = (5 + 20* 6)*4;
		glLoadIdentity();
		glScalef(fX/350,4,1);
		glPushMatrix();
			glTranslatef(0, 0,0);
			m_tower.Draw();
			glTranslatef(0, 10,0);
			m_tilt.Draw();
			glTranslatef(170,10,0);
			m_beep.Draw();
			glTranslatef(20,0,0);
			m_f1.Draw();
			glTranslatef(20,0,0);
			m_f2.Draw();
			glTranslatef(20,0,0);
			m_f3.Draw();
			glTranslatef(20,0,0);
			m_htUV.Draw();
			glTranslatef(20,0,0);
			m_uv.Draw();
			glTranslatef(20,0,0);
			m_tx.Draw();
			glTranslatef(20,0,0);
			m_rx.Draw();
		glPopMatrix();
	}

}; // namespace Boards
