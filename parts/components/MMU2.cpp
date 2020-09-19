/*
	MMU2.cpp - A Missing-materials-unit for MK404

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

#include "MMU2.h"
#include "HC595.h"            // for HC595, TMC2130::IRQ::POSITION_OUT, MMU2...
#include "IKeyClient.h"
#include "LED.h"              // for LED
#include "MM_Control_01.h"    // for MM_Control_01
#include "PinNames.h"         // for Pin::FINDA_PIN
#include "TMC2130.h"          // for TMC2130
#include "gsl-lite.hpp"
#include "uart_pty.h"         // for uart_pty
#include <GL/freeglut_std.h>          // for glutGet, glutStrokeCharacter, GLUT_STRO...
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glTranslatef, glVertex3f, glColor3f
#else
# include <GL/gl.h>           // for glTranslatef, glVertex3f, glColor3f
#endif
#include <cstdlib>           // for exit
#include <cstring>
#include <iostream>            // for fprintf, size_t, stderr


// Yes yes, globals are bad. But we don't have any choice because freeglut calls
// don't have parameter void pointers to retain the c++ class pointer. :-/
MMU2 *MMU2::g_pMMU = nullptr;

using Boards::MM_Control_01;

MMU2::MMU2():IKeyClient(),MM_Control_01()
{
	if (g_pMMU)
	{
		std::cerr << "Error: Cannot have multiple MMU instances due to freeglut limitations\n";
		exit(1);
	}
	g_pMMU = this;
	SetBoardName("MMU2");
	CreateBoard("MM-control-01.hex",0, false, 100,"");

	RegisterKeyHandler('F',"Toggle the FINDA");
	RegisterKeyHandler('A', "Resumes full-auto MMU mode.");
}

void MMU2::OnKeyPress(const Key& key)
{
	switch (key)
	{
		case 'F':
		{
			std::cout << "FINDA toggled (in manual control)\n";
			m_bAutoFINDA = false;
			ToggleFINDA();
		}
		break;
		case 'A':
		{
			std::cout << "FINDA in Auto control\n";
			m_bAutoFINDA = true;
			//FSensorResumeAuto(); // Also restore IR auto handling.
			break;
		}
	}
}

const std::string MMU2::GetSerialPort()
{
	return m_UART.GetSlaveName();
}

void MMU2::Draw(float fY)		/* function called whenever redisplay needed */
{
	//float fY = (float)(glutGet(GLUT_WINDOW_HEIGHT)/4);
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
		m_Extr.Draw_Simple();
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

void MMU2::SetupHardware()
{

	Boards::MM_Control_01::SetupHardware();

	_Init(GetAVR(), this);

	RegisterNotify(RESET,MAKE_C_CALLBACK(MMU2,OnResetIn),this);
	RegisterNotify(PULLEY_IN, MAKE_C_CALLBACK(MMU2,OnPulleyFeedIn),this);
	RegisterNotify(SHIFT_IN, MAKE_C_CALLBACK(MMU2,LEDHandler),this);

	m_Sel.ConnectTo(TMC2130::POSITION_OUT,GetIRQ(SELECTOR_OUT));
	m_Idl.ConnectTo(TMC2130::POSITION_OUT,GetIRQ(IDLER_OUT));
	m_Extr.ConnectTo(TMC2130::POSITION_OUT,GetIRQ(PULLEY_IN));
	m_shift.ConnectTo(HC595::SHIFT_OUT, GetIRQ(SHIFT_IN));
}


void MMU2::OnResetIn(struct avr_irq_t *irq, uint32_t value)
{
	if (!value && !m_bStarted)
	{
		StartAVR();
	}
    else if (irq->value && !value)
	{
        m_bReset = true;
	}
}

void MMU2::ToggleFINDA()
{
		m_bFINDAManual = !m_bFINDAManual;
		std::cout << "FINDA (manual) toggled: " << (m_bFINDAManual?1U:0U) << '\n';
		SetPin(FINDA_PIN, m_bFINDAManual? 1:0);
		RaiseIRQ(FINDA_OUT,m_bFINDAManual? 1 : 0);
}

void MMU2::OnPulleyFeedIn(struct avr_irq_t * ,uint32_t value)
{
	float posOut;
	std::memcpy(&posOut, &value,4);

	if (m_bAutoFINDA)
	{
   		SetPin(FINDA_PIN,posOut>33.0f);
		// Reflect the distance out for IR sensor triggering.
		RaiseIRQ(FEED_DISTANCE, value);
		RaiseIRQ(FINDA_OUT,posOut>33.f);
	}
	else
	{
		SetPin(FINDA_PIN, m_bFINDAManual? 1:0);
		RaiseIRQ(FINDA_OUT,m_bFINDAManual? 1 : 0);
	}

}

void MMU2::LEDHandler(avr_irq_t *, uint32_t value)
{
	uint32_t valOut = 0;
	valOut = (value >>6U) & 0b1111111111U; // Just the LEDs.
	if (GetIRQ(LEDS_OUT)->value != valOut)
	{
		RaiseIRQ(LEDS_OUT,valOut);
	}
}
