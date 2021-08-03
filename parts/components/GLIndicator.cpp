/*
	GLIndicator.h - Simple GLIndicator visualizer.

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


#include "GLIndicator.h"
#include <GL/freeglut_std.h>          // for glutStrokeCharacter, GLUT_STROKE_MONO_R...
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glVertex2f, glBegin, glColor3f, glEnd
#else
# include <GL/gl.h>           // for glVertex2f, glBegin, glColor3f, glEnd
#endif

GLIndicator::GLIndicator(char chrLabel, bool bInvert, bool bNoBlack):m_chrLabel(chrLabel),m_bInvert(bInvert),m_bNoBlackBG(bNoBlack)
{
	m_uiBrightness = 255*bInvert;
}

void GLIndicator::Draw()
{
	if (!m_bVisible)
	{
		return;
	}
	bool m_bOn = m_uiBrightness>0;
	uint16_t uiBrt = m_uiBrightness;
	if (m_bNoBlackBG)
	{
		m_uiBrightness = ((m_uiBrightness*9)/10)+25;
	}
    glPushMatrix();
        if (m_bOn)
		{
            glColor3us(m_Color.red*uiBrt, m_Color.green*uiBrt, m_Color.blue*uiBrt);
		}
        else
		{
            glColor3ub(m_Color.red/10, m_Color.green/10, m_Color.blue/10);
		}

        glBegin(GL_QUADS);
            glVertex2f(0,10);
            glVertex2f(20,10);
            glVertex2f(20,0);
            glVertex2f(0,0);
        glEnd();
		bool bWhite = m_uiBrightness<128;
        glColor3f(bWhite,bWhite,bWhite);
        glTranslatef(4,7,-1);
        glScalef(0.1,-0.05,1);
		glPushMatrix();
			auto uiRot = m_uiRot.load();
			if (uiRot)
			{
				glTranslatef(50,50,0);
				glRotatef(m_uiRot,0,0,-1);
				glTranslatef(-50,-50,0);
			}
        	glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,m_chrLabel);
		glPopMatrix();
    glPopMatrix();
}

void GLIndicator::SetValue(uint8_t value) {
	if (m_bInvert)
	{
		m_uiBrightness = 255U-value;
	}
	else
	{
		m_uiBrightness = value;
	}
}
