/*
	Beeper.cpp - Beeper visualizer for MK3Sim

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK3SIM.

	MK3SIM is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK3SIM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK3SIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Beeper.h"
#include "stdio.h"
#include "GL/glut.h"

Beeper::Beeper():SoftPWMable(true,this, 1)
{
}

void Beeper::OnDigitalChange(avr_irq_t *irq, uint32_t value)
{
	//printf("Beeper turned on: %d\n", value);
}

void Beeper::OnPWMChange(avr_irq_t* irq, uint32_t value)
{
	m_uiFreq = value;
	//printf("Beeper PWM change: %d\n", value);
}

void Beeper::Init(avr_t *avr)
{
    _Init(avr, this);
	Beeper::RegisterNotify(DIGITAL_IN,MAKE_C_CALLBACK(Beeper,OnDigitalInSPWM), this);
}

void Beeper::Draw()
{
	bool m_bOn = m_uiFreq>0;
	uint16_t uiBrt = ((m_uiFreq*9)/10)+25;
    glPushMatrix();
        if (m_bOn)
            glColor3us(255*uiBrt, 128*uiBrt, 0);
        else
            glColor3ub(25,12,0);

        glBegin(GL_QUADS);
            glVertex2f(0,10);
            glVertex2f(20,10);
            glVertex2f(20,0);
            glVertex2f(0,0);
        glEnd();
        glColor3f(!m_bOn,!m_bOn,!m_bOn);
        glTranslatef(4,7,-1);
        glScalef(0.1,-0.05,1);
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,'T');
    glPopMatrix();
}
