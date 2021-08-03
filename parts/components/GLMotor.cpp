/*
	GLMotor.cpp - Lightweight motor drawable.

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

#include "GLMotor.h"

#include <GL/freeglut_std.h>          // for glutStrokeCharacter, GLUT_STROKE_MONO_R...
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glVertex3f, glColor3f, glBegin, glEnd
#else
# include <GL/gl.h>           // for glVertex3f, glColor3f, glBegin, glEnd
#endif
#include <atomic>
#include <cstring>				// for memcpy
#include <string>

void GLMotor::Draw()
{
	if (!m_bConfigured) {
		return;
	}
		// Copy atomic to local
	float fPos = m_fCurPos;
	if (m_bDrawStall)
	{
		glColor3f(0.7,0,0);
	}
	else
	{
		glColor3f(0,0,0);
	}
	glBegin(GL_QUADS);
		glVertex3f(0,0,0);
		glVertex3f(350,0,0);
		glVertex3f(350,10,0);
		glVertex3f(0,10,0);
		if (m_bStealthMode)
		{
			glColor3f(0.9,1,0.4); // acid green is the new "stealth"
		}
		else
		{
			glColor3f(1,1,1);
		}
		if (m_bEnable)
		{
			glVertex3f(3,8,0);
			glVertex3f(13,8,0);
			glVertex3f(13,1,0);
			glVertex3f(3,1,0);
			glColor3f(0,0,0);
		}
	glEnd();
	glPushMatrix();
		glTranslatef(3,7,0);
		glScalef(0.09,-0.05,0);
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,m_cAxis);
		//glTranslatef(  bIsSimple? 30 : 280 ,7,0);
		//glScalef(0.09,-0.05,0);
		// Values translated according to existing Scalef()
		glTranslatef(m_bIsSimple? 195 : 2973 ,0,0);
		glColor3f(1,1,1);
		std::string strPos = std::to_string(fPos);
		for (int i=0; i<std::min(7,static_cast<int>(strPos.size())); i++)
		{
			glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,strPos[i]);
		}
	glPopMatrix();
	if (m_bIsSimple)
	{
		return;
	}
	glPushMatrix();
		glTranslatef(20,0,0);
		glColor3f(1,0,0);
		glBegin(GL_QUADS);
			glVertex3f(0,2,0);
			glVertex3f(-2,2,0);
			glVertex3f(-2,8,0);
			glVertex3f(0,8,0);
			glVertex3f(m_fEnd,2,0);
			glVertex3f(m_fEnd+2,2,0);
			glVertex3f(m_fEnd+2,8,0);
			glVertex3f(m_fEnd,8,0);
			glColor3f(0,1,1);
			glVertex3f(fPos-0.5,2,0);
			glVertex3f(fPos+0.5,2,0);
			glVertex3f(fPos+0.5,8,0);
			glVertex3f(fPos-0.5,8,0);
		glEnd();
	glPopMatrix();
}


float GLMotor::StepToPos(int32_t step)
{
	// Position is always in 16ths of a step.
	return static_cast<float>(step)/static_cast<float>(m_uiStepsPerMM);
}

int32_t GLMotor::PosToStep(float pos)
{
	// Position is always 16ths of a step...
	return pos*static_cast<float>(m_uiStepsPerMM);
}
