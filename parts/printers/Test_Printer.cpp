/*
	Part_Test.cpp - Printer definition for the part test printer
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

#include "Test_Printer.h"
#include "A4982.h"
#include "Beeper.h"
#include "Fan.h"              // for Fan
#include "GLHelper.h"
#include "HD44780GL.h"
#include "Heater.h"           // for Heater
#include "LED.h"
#include "MMU1.h"
#include "TMC2130.h"          // for TMC2130
#include <GL/glew.h> //NOLINT - must come first.

void Test_Printer::SetupHardware()
{
	Test_Board::SetupHardware();
}

void Test_Printer::OnAVRCycle()
{
}

void Test_Printer::Draw()
{
		glPushMatrix();
		glLoadIdentity(); // Start with an identity matrix
			glScalef(4, 4, 1);

			m_lcd.Draw(0x00FF00FF, /* background */
						0x0000FFFF, /* character background */
						0xFFFFFFFF, /* text */
						0x00000055 /* shadow */ );
		glPopMatrix();
		glLoadIdentity();
		float fX = (5 + m_lcd.GetWidth()* 6)*4;
		float fY = (5 + m_lcd.GetHeight() * 9);
		glScalef(fX/350,4,1);
		glTranslatef(0, fY,0);
		m_TMC.Draw();
		glTranslatef(0,10,0);
		m_TMC.Draw_Simple();
		glTranslatef(0,10,0);
		glPushMatrix();
			m_Fan.Draw();
			glTranslatef(20,0,0);
			m_heat.Draw();
			glTranslatef(20,0,0);
			m_LED.Draw();
			glTranslatef(20,0,0);
			m_LED2.Draw();
			glTranslatef(20,0,0);
			m_buzzer.Draw();
			glTranslatef(20,0,0);
			m_MM1.Draw();
		glPopMatrix();
		glTranslatef(0,10,0);
		m_Allg.Draw();
		glTranslatef(0,10,0);
		m_Allg.Draw_Simple();
		m_gl.OnDraw();
}
