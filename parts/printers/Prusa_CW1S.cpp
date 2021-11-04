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
#include "MK3SGL.h"
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
	if ((GetVisualType()!="none") && m_pVis)
	{
		m_pVis->FlagForRedraw();
	}
}

void Prusa_CW1S::OnVisualTypeSet(const std::string &type)
{
	if (type == "lite" || type == "fancy")
	{
		m_pVis.reset(new MK3SGL("cw1s_" + type,false,this)); //NOLINT - suggestion is c++14.

		AddHardware(*m_pVis);

		m_pVis->ConnectFrom(m_tmc.GetIRQ(TMC2130::POSITION_OUT),MK3SGL::E_IN);
		m_pVis->ConnectFrom(m_f1.GetIRQ(Fan::ROTATION_OUT), MK3SGL::EFAN_IN);
		m_pVis->ConnectFrom(m_f2.GetIRQ(Fan::ROTATION_OUT), MK3SGL::PFAN_IN);
		avr_raise_irq(m_pVis->GetIRQ(MK3SGL::GENERIC_3),1);
		avr_raise_irq(m_pVis->GetIRQ(MK3SGL::GENERIC_2),1);
		m_pVis->ConnectFrom(m_lid.GetIRQ(Button::BUTTON_OUT), MK3SGL::GENERIC_3);
		m_pVis->ConnectFrom(m_tank.GetIRQ(Button::BUTTON_OUT), MK3SGL::GENERIC_2);
		m_pVis->ConnectFrom(m_gpio.GetIRQ(MCP23S17::MCP_GPA6), MK3SGL::GENERIC_1);

		m_pVis->SetLCD(&m_lcd);

		m_pVis->SetStepsPerMM(
			0,0,0,
			m_tmc.GetConfig().uiFullStepsPerMM
		);
	}
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
