/*
	Heater.cpp - a heater object for MK404. There's not much to it,
    it just ticks the temperature "up" at a determined rate when active on PWM and down in
    in an exponential curve when off.

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

#include "Heater.h"
#include "TelemetryHost.h"
#include "sim_regbit.h"       // for avr_regbit_get, AVR_IO_REGBIT
#include <GL/freeglut_std.h>          // for glutStrokeCharacter, GLUT_STROKE_MONO_R...
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glVertex2f, glBegin, glColor3f, glColor3fv
#else
# include <GL/gl.h>           // for glVertex2f, glBegin, glColor3f, glColor3fv
#endif
#include <cmath>             // for pow

#define TRACE(_w)
#ifndef TRACE
#include <stdio.h>
#define TRACE(_w)_w
#endif


avr_cycle_count_t Heater::OnTempTick(avr_t * pAVR, avr_cycle_count_t)
{
	if (m_bStopTicking)
	{
		return 0;
	}

    if (m_uiPWM>0 || (pAVR->cycle-m_cntOff)<(pAVR->frequency/100))
    {
        float fDelta = (m_fThermalMass*(static_cast<float>(m_uiPWM)/255.0f))*0.3f;
        m_fCurrentTemp += fDelta;
    }
    else // Cooling - do a little exponential decay
    {
        float dT = (m_fCurrentTemp - m_fAmbientTemp)*pow(2.7183,-0.005*0.3);
        m_fCurrentTemp -= m_fCurrentTemp - (m_fAmbientTemp + dT);
    }
	m_iDrawTemp = m_fCurrentTemp;

    TRACE(printf("New temp value: %.02f\n",m_fCurrentTemp));
    RaiseIRQ(TEMP_OUT,static_cast<int>(m_fCurrentTemp*256.f));

    if (m_uiPWM>0 || m_fCurrentTemp>m_fAmbientTemp+0.3)
	{
        RegisterTimerUsec(m_fcnTempTick,300000,this);
	}
    else
    {
        m_fCurrentTemp = m_fAmbientTemp;
        RaiseIRQ(TEMP_OUT,static_cast<int>(m_fCurrentTemp*256.f));
    }
    return 0;
}


void Heater::OnPWMChanged(struct avr_irq_t *,uint32_t value)
{
    if (m_bAuto) // Only update if auto (pwm-controlled). Else user supplied RPM.
	{
        m_uiPWM = value;
	}
	TRACE(printf("New PWM: %02x\n",value));
    if (m_uiPWM > 0)
	{
        RegisterTimerUsec(m_fcnTempTick, 100000, this);
	}
	else
	{
		m_cntOff = m_pAVR->cycle;
	}

    if (GetIRQ(ON_OUT)->value != (m_uiPWM>0))
	{
        RaiseIRQ(ON_OUT,m_uiPWM>0);
	}
}

//TCCR0A  _SFR_IO8(0x24)
//#define COM0B0  4

void Heater::OnDigitalChanged(struct avr_irq_t * irq, uint32_t value)
{

    if (m_bIsBed) // The heatbed PWM is based on inverting mode trickery. We can just watch COM0B0 rather than the digital pin output.
    {
        avr_regbit_t inv = AVR_IO_REGBIT(0x24 + 32,4);
        uint8_t COM0B0 = avr_regbit_get(m_pAVR, inv);
        value = COM0B0^1U;
    }

    if (value==1)
	{
        value = 255;
	}

    OnPWMChanged(irq,value);
}

Heater::Heater(float fThermalMass, float fAmbientTemp, bool bIsBed,
			   char chrLabel, float fColdTemp, float fHotTemp):
			   								Scriptable(std::string("Heater_") + chrLabel),
                                            m_fThermalMass(fThermalMass),
                                            m_fAmbientTemp(fAmbientTemp),
                                            m_fCurrentTemp(fAmbientTemp),
                                            m_bIsBed(bIsBed),
                                            m_chrLabel(chrLabel),
                                            m_fColdTemp(fColdTemp),
                                            m_fHotTemp(fHotTemp)
{
	RegisterAction("SetPWM","Sets the raw heater PWM value",ActSetPWM, {ArgType::Int});
	RegisterActionAndMenu("Resume", "Resumes auto PWM control and clears the 'stopheating' flag",ActResume);
	RegisterActionAndMenu("StopHeating","Stops heating, as if a thermal runaway is happening due to loose heater or thermistor",ActStopHeating);
}

Scriptable::LineStatus Heater::ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs)
{
	switch (iAct)
	{
		case ActSetPWM:
		{
			uint8_t uiVal = stoi(vArgs.at(0));
			Set(uiVal);
			return LineStatus::Finished;
		}
		case ActResume:
			Resume_Auto();
			return LineStatus::Finished;
		case ActStopHeating:
			m_bStopTicking = true;
			return LineStatus::Finished;

	}
	return LineStatus::Unhandled;
}

void Heater::Init(struct avr_t * avr, avr_irq_t *irqPWM, avr_irq_t *irqDigital)
{
    _Init(avr, this);
    if(irqPWM) ConnectFrom(irqPWM, PWM_IN);
    if(irqDigital) ConnectFrom(irqDigital, DIGITAL_IN);

    RegisterNotify(PWM_IN, MAKE_C_CALLBACK(Heater,OnPWMChanged),this);
    RegisterNotify(DIGITAL_IN, MAKE_C_CALLBACK(Heater,OnDigitalChanged),this);


	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, PWM_IN, {TC::Heater,TC::PWM},8);
	TH.AddTrace(this, DIGITAL_IN, {TC::Heater});
	TH.AddTrace(this, ON_OUT, {TC::Heater,TC::Misc});
	TH.AddTrace(this, TEMP_OUT, {TC::Heater});

  	RaiseIRQ(TEMP_OUT,static_cast<int>(m_fCurrentTemp*256.f));
}

void Heater::Set(uint8_t uiPWM)
{
    m_bAuto = false;
    m_uiPWM = uiPWM;
    RaiseIRQ(PWM_IN,0XFF);
}

void Heater::Resume_Auto()
{
    m_bAuto = true;
	m_bStopTicking = false;
	RaiseIRQ(PWM_IN,m_uiPWM);
}


constexpr Color3fv Heater::m_colColdTemp;
constexpr Color3fv Heater::m_colHotTemp;

void Heater::Draw()
{
	bool bOn = m_uiPWM>0;

	Color3fv colFill;
	float v = (float(m_iDrawTemp) - m_fColdTemp) / (m_fHotTemp - m_fColdTemp);
	colorLerp(m_colColdTemp, m_colHotTemp, v, colFill);

    glPushMatrix();
	    glColor3fv(static_cast<float*>(colFill));
        glBegin(GL_QUADS);
            glVertex2f(0,10);
            glVertex2f(20,10);
            glVertex2f(20,0);
            glVertex2f(0,0);
        glEnd();
        glColor3f(bOn,bOn,bOn);
        glTranslatef(8,7,-1);
        glScalef(0.05,-0.05,1);
		glPushAttrib(GL_LINE_BIT);
			glLineWidth(3);
			glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,m_chrLabel);
		glPopAttrib();
    glPopMatrix();
}
