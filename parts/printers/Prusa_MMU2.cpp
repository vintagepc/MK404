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
	float fY = (float)(glutGet(GLUT_WINDOW_HEIGHT)/4);
	glScalef(500.F/350.F, 4.F, 1.F);
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
		// for (auto &c : m_strTitle)
		// {
		// 	glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,c);
		// }
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

void Prusa_MMU2::OnVisualTypeSet(const std::string &type)
{
	if (type!="lite")
	{
		return;
	}

	m_pVis.reset(new MK3SGL(type,true,this)); //NOLINT - suggestion is c++14.

	AddHardware(*m_pVis);
	// Wire up the additional MMU stuff.

	//AddHardware(m_sniffer,'2');
	//m_pVis->ConnectFrom(m_sniffer.GetIRQ(GCodeSniffer::CODEVAL_OUT),MK3SGL::TOOL_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::SELECTOR_OUT), MK3SGL::SEL_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::IDLER_OUT), MK3SGL::IDL_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::LEDS_OUT),MK3SGL::MMU_LEDS_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::FINDA_OUT),MK3SGL::FINDA_IN);
	//m_pVis->ConnectFrom(GetIRQ(MMU2::FEED_DISTANCE), MK3SGL::FEED_IN);
}
