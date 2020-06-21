/*
    Fan.cpp - Simple fan tach sim for Einsy Rambo

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
#include "Fan.h"
#include "stdio.h"
#include <sim_time.h>
#include <GL/glut.h>
//#define TRACE(_w)_w
#ifndef TRACE
#define TRACE(_w)
#endif

Fan::Fan(uint16_t iMaxRPM, char chrSym, bool bIsSoftPWM):m_uiMaxRPM(iMaxRPM),m_chrSym(chrSym),SoftPWMable(bIsSoftPWM,this),Scriptable("Fan")
{
	RegisterAction("Stall", "Stalls the fan", Actions::Stall);
	RegisterAction("Resume","Resumes fan from a stall condition",Actions::Resume);
}

avr_cycle_count_t Fan::OnTachChange(avr_t * avr, avr_cycle_count_t when)
{
    RaiseIRQ(TACH_OUT, m_bPulseState^=1);
    RegisterTimerUsec(m_fcnTachChange,m_uiUsecPulse,this);
    return 0;
}

void Fan::Draw()
{
	bool bOn = m_uiPWM>0;

    glPushMatrix();
	    glColor3ub(0,m_uiPWM>>1,0);
        glBegin(GL_QUADS);
            glVertex2f(0,10);
            glVertex2f(20,10);
            glVertex2f(20,0);
            glVertex2f(0,0);
        glEnd();
        glColor3f(1,1,1);
        glTranslatef(9,5,-1);
        glScalef(0.10,-0.05,1);
		if (bOn)
			m_uiRot = (m_uiRot + (2*(m_uiPWM)/10))%360;
		glRotatef(m_uiRot,0,0,-1);
		glTranslatef(-50,-50,0);
		glPushAttrib(GL_LINE_BIT);
			glLineWidth(3);
			glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,m_chrSym);
		glPopAttrib();
    glPopMatrix();
}


Scriptable::LineStatus Fan::ProcessAction(unsigned int ID, const vector<string> &vArgs)
{
	switch (ID)
	{
		case Actions::Stall:
			Set(0);
			return LineStatus::Finished;
		case Actions::Resume:
			Resume_Auto();
			return LineStatus::Finished;
	}
}

void Fan::OnPWMChange(struct avr_irq_t * irq, uint32_t value)
{
    m_uiPWM = value;
    if (m_bAuto) // Only update RPM if auto (pwm-controlled). Else user supplied RPM.
        m_uiCurrentRPM = ((m_uiMaxRPM)*value)/255;

    RaiseIRQ(SPEED_OUT,m_uiCurrentRPM);

    float fSecPerRev = 60.0f/(float)m_uiCurrentRPM;
    float fuSPerRev = 1000000*fSecPerRev;
    m_uiUsecPulse = fuSPerRev/4; // 4 pulses per rev.
    TRACE(printf("New PWM(%u)/RPM/cyc: %u / %u / %u\n", m_uiMaxRPM, m_uiPWM, m_uiCurrentRPM, m_uiUsecPulse));
    if (m_uiCurrentRPM>0)
    {
        RegisterTimerUsec(m_fcnTachChange,m_uiUsecPulse,this);
    }
    else
    {
        CancelTimer(m_fcnTachChange,this);
    }
}

// Just a dummy wrapper to handle non-PWM control (digitalWrite)
void Fan::OnDigitalChange(struct avr_irq_t * irq, uint32_t value)
{
   	OnPWMChange(irq, value*0xFF);
}

void Fan::Init(struct avr_t *avr, avr_irq_t *irqTach, avr_irq_t *irqDigital, avr_irq_t *irqPWM)
{
    _Init(avr, this);

    if(irqPWM)  ConnectFrom(irqPWM, PWM_IN);
    if(irqDigital) ConnectFrom(irqDigital, DIGITAL_IN);
    if(irqTach) ConnectTo(TACH_OUT,irqTach);

    RegisterNotify(PWM_IN, MAKE_C_CALLBACK(Fan,OnPWMChange), this);
    RegisterNotify(DIGITAL_IN,MAKE_C_CALLBACK(Fan,OnDigitalInSPWM), this);

}

void Fan::Set(uint16_t iRPM)
{
    m_bAuto = false;
    m_uiCurrentRPM = iRPM;
    RaiseIRQ(PWM_IN,0XFF);
}

void Fan::Resume_Auto()
{
    m_bAuto = true;
}
