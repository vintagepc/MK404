/*
	Prusa_MMU2.cpp - Standalone MMU2 sim for MK404
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

#include "Prusa_MMU2.h"
#include "GLHelper.h"
#include "MMU2.h"
#include "MK3SGL.h"

// void Prusa_MMU2::SetupHardware()
// {
// 	_Init(Board::m_pAVR,this);
// 	m_MMU.StartAVR();
// }


std::pair<int,int> Prusa_MMU2::GetWindowSize()
{
	return {125,50};
}


void Prusa_MMU2::Draw()
{
	glScalef(50.F/35.F,4,1);
	float fY = (float)(glutGet(GLUT_WINDOW_HEIGHT)/4);
	MM_Control_01::Draw(fY);
	m_gl.OnDraw();
}

void Prusa_MMU2::OnVisualTypeSet(const std::string &type)
{
	// if (type=="lite")
	// {
	// 	m_pVis.reset(new MK3SGL(type,true,this)); //NOLINT - suggestion is c++14.
	// 	AddHardware(*m_pVis);
	// }


	// Wire up the additional MMU stuff.

	//AddHardware(m_sniffer,'2');
	//m_pVis->ConnectFrom(m_sniffer.GetIRQ(GCodeSniffer::CODEVAL_OUT),MK3SGL::TOOL_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::SELECTOR_OUT), MK3SGL::SEL_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::IDLER_OUT), MK3SGL::IDL_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::LEDS_OUT),MK3SGL::MMU_LEDS_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::FINDA_OUT),MK3SGL::FINDA_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::FEED_DISTANCE), MK3SGL::FEED_IN);
}
