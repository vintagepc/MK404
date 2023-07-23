/*
	MM_Control_01.cpp - Board definition for the Prusa MMU2
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

#include "MM_Control_01.h"
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

	MM_Control_01::~MM_Control_01()
	{
		#ifndef __APPLE__
			pthread_cancel(m_usb_thread);
			usbip_destroy(m_usb);
		#endif
	}

	void MM_Control_01::OnAVRReset()
	{
		if (m_bSidebandInit)
		{
			std::cout << "Sideband reset complete\n";
			m_USideband.BypassXON();
		}
	}

	void MM_Control_01::SetupSideband()
	{
		m_USideband.Init(m_pAVR);
		m_USideband.ConnectPTYOnly("/tmp/MK404-MMU-sideband");
		m_bSidebandInit = true;
	}

	void MM_Control_01::SetupHardware()
	{
		DisableInterruptLevelPoll(5);

		AddSerialPty(&m_UART,'1');

		AddUARTTrace('1');

		AddHardware(m_shift);
		TryConnect(SHIFT_LATCH,&m_shift,HC595::IN_LATCH);
		TryConnect(SHIFT_DATA,&m_shift,HC595::IN_DATA);
		TryConnect(SHIFT_CLOCK,&m_shift,HC595::IN_CLOCK);

		TMC2130::TMC2130_cfg_t cfg;
		cfg.uiFullStepsPerMM = 84*32;
		cfg.fStartPos = 0;
		cfg.bHasNoEndStops = true;

		m_Extr.SetConfig(cfg);
		AddHardware(m_Extr);
		TryConnect(P_TMC2130_CS, &m_Extr, TMC2130::SPI_CSEL);
		TryConnect(P_STEP_PIN, &m_Extr, TMC2130::STEP_IN);
		TryConnect(&m_Extr,TMC2130::DIAG_OUT,P_TMC2130_DIAG);
		SetPin(P_TMC2130_DIAG,0);
		m_Extr.ConnectFrom(m_shift.GetIRQ(HC595::BIT0),	TMC2130::DIR_IN);
		m_Extr.ConnectFrom(m_shift.GetIRQ(HC595::BIT1),	TMC2130::ENABLE_IN);
		m_Extr.SetSimple(true);


		cfg.uiFullStepsPerMM = 200*32;
		cfg.iMaxMM = 76;
		cfg.fStartPos = 10;
		cfg.bInverted = true;
		cfg.bHasNoEndStops = false;
		m_Sel.SetConfig(cfg);
		AddHardware(m_Sel);
		TryConnect(S_TMC2130_CS,&m_Sel,TMC2130::SPI_CSEL);
		TryConnect(S_STEP_PIN,&m_Sel,TMC2130::STEP_IN);
		TryConnect(&m_Sel,TMC2130::DIAG_OUT,S_TMC2130_DIAG);
		SetPin(S_TMC2130_DIAG,0);
		m_Sel.ConnectFrom(m_shift.GetIRQ(HC595::BIT2), TMC2130::DIR_IN);
		m_Sel.ConnectFrom(m_shift.GetIRQ(HC595::BIT3), TMC2130::ENABLE_IN);

		cfg.uiFullStepsPerMM = 9*16;
		cfg.iMaxMM = 225;
		m_Idl.SetConfig(cfg);
		AddHardware(m_Idl);
		TryConnect(I_TMC2130_CS,&m_Idl,TMC2130::SPI_CSEL);
		TryConnect(I_STEP_PIN,&m_Idl,TMC2130::STEP_IN);
		TryConnect(&m_Idl,TMC2130::DIAG_OUT,I_TMC2130_DIAG);
		SetPin(I_TMC2130_DIAG,0);
		m_Idl.ConnectFrom(m_shift.GetIRQ(HC595::BIT5), TMC2130::ENABLE_IN);
		m_Idl.ConnectFrom(m_shift.GetIRQ(HC595::BIT4), TMC2130::DIR_IN);


		for (int i=0; i<5; i++)
		{
//			m_lGreen[i] = {0x00FF00FF};
			AddHardware(gsl::at(m_lGreen,i));
//			m_lRed[i] = {0xFF0000FF};
			AddHardware(gsl::at(m_lRed,i));
		}
//		m_lFINDA = LED(0xFFCC00FF,'F');
		AddHardware(m_lFINDA);
		TryConnect(FINDA_PIN,&m_lFINDA,LED::LED_IN);
		SetPin(FINDA_PIN,0);

		m_lGreen[0].ConnectFrom(m_shift.GetIRQ(HC595::BIT6), LED::LED_IN);
		m_lRed[0].ConnectFrom(	m_shift.GetIRQ(HC595::BIT7), LED::LED_IN);
		m_lGreen[4].ConnectFrom(m_shift.GetIRQ(HC595::BIT8), LED::LED_IN);
		m_lRed[4].ConnectFrom(	m_shift.GetIRQ(HC595::BIT9), LED::LED_IN);
		m_lGreen[3].ConnectFrom(m_shift.GetIRQ(HC595::BIT10), LED::LED_IN);
		m_lRed[3].ConnectFrom(	m_shift.GetIRQ(HC595::BIT11), LED::LED_IN);
		m_lGreen[2].ConnectFrom(m_shift.GetIRQ(HC595::BIT12), LED::LED_IN);
		m_lRed[2].ConnectFrom(	m_shift.GetIRQ(HC595::BIT13), LED::LED_IN);
		m_lGreen[1].ConnectFrom(m_shift.GetIRQ(HC595::BIT14), LED::LED_IN);
		m_lRed[1].ConnectFrom(	m_shift.GetIRQ(HC595::BIT15), LED::LED_IN);

		AddHardware(m_buttons,5);
		#ifndef __APPLE__
			m_usb = usbip_create(m_pAVR);
			// pragma: LCOV_EXCL_START
			if (!m_usb)
			{
				std::cout << "Failed to create USBIP context\n";
				exit(1);
			}
			// pragma: LCOV_EXCL_STOP
			pthread_create(&m_usb_thread, nullptr, usbip_main, m_usb);
		#endif

	}

	void MM_Control_01::Draw(float fY)		/* function called whenever redisplay needed */
	{
		glPushMatrix();
			glColor3f(0,0,0);
			glTranslatef(0,fY-50,0);
			glBegin(GL_QUADS);
				glVertex3f(0,0,0);
				glVertex3f(350,0,0);
				glVertex3f(350,10,0);
				glVertex3f(0,10,0);
			glEnd();
			glTranslatef(20,7,0);
			glColor3f(1,1,1);
			glScalef(0.09,-0.05,0);
			for (auto &c : m_strTitle)
			{
				glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,c);
			}
		glPopMatrix();
		glPushMatrix();
			glColor3f(0,0,0);
			glTranslatef(0,fY-40,0);
			m_Sel.Draw();
		glPopMatrix();
		glPushMatrix();
			glColor3f(0,0,0);
			glTranslatef(0,fY-30,0);
			m_Idl.Draw();
		glPopMatrix();
		glPushMatrix();
			glColor3f(0,0,0);
			glTranslatef(0,fY-20,0);
			m_Extr.Draw();
		glPopMatrix();
		glPushMatrix();
			glColor3f(0,0,0);
			glTranslatef(0,fY-10,0);
			glBegin(GL_QUADS);
				glVertex3f(0,0,0);
				glVertex3f(350,0,0);
				glVertex3f(350,10,0);
				glVertex3f(0,10,0);
			glEnd();
			for (int i=0; i<5; i++)
			{
				gsl::at(m_lRed,i).Draw();
				glTranslatef(20,0,0);
				gsl::at(m_lGreen,i).Draw();
				glTranslatef(40,0,0);
			}
			m_lFINDA.Draw();
		glPopMatrix();
	}

}; // namespace Boards
