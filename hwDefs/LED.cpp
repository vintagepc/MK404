/*
	LED.h - Simple LED visualizer.

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


#include "GL/glut.h"
#include "LED.h"

LED::LED(uint32_t uiHexColor, char chrLabel):m_chrLabel(chrLabel)
{
    m_fColor[0] = (float)(uiHexColor >> 24)/255.0f;
    m_fColor[1] = (float)((uiHexColor >> 16)& 0xFF)/255.0f;
    m_fColor[2] = (float)((uiHexColor >> 8)& 0xFF)/255.0f;
}


void LED::OnValueChanged(struct avr_irq_t * irq, uint32_t value)
{
	m_bOn = value;
}

void LED::Draw()
{
    glPushMatrix();
        if (m_bOn)
            glColor3f(m_fColor[0], m_fColor[1], m_fColor[2]);
        else
            glColor3f(m_fColor[0]/10, m_fColor[1]/10, m_fColor[2]/10);

        glBegin(GL_QUADS);
            glVertex2f(0,10);
            glVertex2f(10,10);
            glVertex2f(10,0);
            glVertex2f(0,0);
        glEnd();
        glColor3f(!m_bOn,!m_bOn,!m_bOn);
        glTranslatef(3,7,-1);
        glScalef(0.05,-0.05,1);
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,m_chrLabel);
    glPopMatrix();
}
void LED::Init(avr_t *avr)
{
    _Init(avr, this);
    RegisterNotify(LED_IN,MAKE_C_CALLBACK(LED,OnValueChanged),this);
}
