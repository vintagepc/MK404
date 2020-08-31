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
#include "HD44780GL.h"
#include <GL/glew.h>
#include <GL/freeglut_std.h>
#include <utility>

void Test_Printer::SetupHardware()
{
	auto prSize = GetWindowSize();
	m_gl.SetWindowHeight(prSize.first*4, prSize.second*4);
	Test_Board::SetupHardware();
}

void Test_Printer::OnAVRCycle()
{
	// int key = m_key;                            // copy atomic to local
	// if (key)
	// {
	// 	m_key = 0;
	// }
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
		glutSwapBuffers();
		m_gl.OnDraw();
}
