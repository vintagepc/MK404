/*
	Prusa_CW1S.cpp - Printer definition for the CW1S
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

#include "Prusa_CW1S.h"
#include "GLHelper.h"
#include <GL/glew.h>		// NOLINT must come before freeglut
#include <GL/freeglut_std.h>  // for GLUT_DOWN, GLUT_LEFT_BUTTON, GLUT_RIGHT...


std::pair<int,int> Prusa_CW1S::GetWindowSize()
{
	return {125,50};
}


void Prusa_CW1S::Draw()
{
	glScalef(50.F/35.F,4,1);
	CW1S::Draw();
	m_gl.OnDraw();
}

void Prusa_CW1S::OnAVRCycle()
{
	int mouseBtn = m_mouseBtn;                  // copy atomic to local
	if (mouseBtn)
	{
		switch (mouseBtn){
			case 1:
				m_enc.MousePush();
				break;
			case 2:
				m_enc.Release();
				break;
			case 3:
				m_enc.Twist(RotaryEncoder::CCW_CLICK);
				break;
			case 4:
				m_enc.Twist(RotaryEncoder::CW_CLICK);
				break;
		}
		m_mouseBtn = 0;
	}
}

void Prusa_CW1S::OnMousePress(int button, int action, int, int)
{
	if (button == GLUT_LEFT_BUTTON || button == GLUT_RIGHT_BUTTON) {
		if (action == GLUT_DOWN) {
			m_mouseBtn = 1;
		} else if (action == GLUT_UP) {
			m_mouseBtn = 2;
		}
	}
	if ((button==3 || button==4) && action == GLUT_DOWN) // wheel
	{
		m_mouseBtn = button;
	}
}
