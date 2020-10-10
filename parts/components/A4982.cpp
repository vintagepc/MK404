/*
	A4982.cpp - Simulated Allegro driver used in the MiniRambo

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

#include "A4982.h"
#include "TelemetryHost.h"

#include <GL/freeglut_std.h>          // for glutStrokeCharacter, GLUT_STROKE_MONO_R...
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glVertex3f, glColor3f, glBegin, glEnd
#else
# include <GL/gl.h>           // for glVertex3f, glColor3f, glBegin, glEnd
#endif
#include <algorithm>          // for min
#include <atomic>
#include <cstring>				// for memcpy
#include <iostream>            // for printf

#define TRACE(x)

void A4982::Draw()
{
	_Draw(false);
}

void A4982::Draw_Simple()
{
	_Draw(true);
}

void A4982::_Draw(bool bIsSimple)
{
	if (!m_bConnected) return;
        glColor3f(0,0,0);
	    glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
        	glColor3f(1,1,1);
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
        glPopMatrix();
        glColor3f(1,1,1);
        glPushMatrix();
            glTranslatef(  bIsSimple? 30 : 280 ,7,0);
            glScalef(0.09,-0.05,0);
            std::string strPos = std::to_string(m_fCurPos);
            for (int i=0; i<std::min(7,static_cast<int>(strPos.size())); i++)
			{
                glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,strPos[i]);
			}
        glPopMatrix();
		if (bIsSimple)
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
				glVertex3f(m_fCurPos-0.5,2,0);
				glVertex3f(m_fCurPos+0.5,2,0);
				glVertex3f(m_fCurPos+0.5,8,0);
				glVertex3f(m_fCurPos-0.5,8,0);
			glEnd();
		glPopMatrix();
}

// Called when DIR pin changes.
void A4982::OnDirIn(struct avr_irq_t * /*irq*/, uint32_t value)
{
    TRACE( cout << "A4982 " << m_cAxis.load() << ": DIR changed to" << value << '\n' );
    m_bDir = value^m_cfg.bInverted; // XOR
}

// Called when STEP is triggered.
void A4982::OnStepIn(struct avr_irq_t * /*irq*/, uint32_t value)
{
	if (!m_bEnable || m_bSleep || m_bReset)
	{
		std::cout << "A4982: STEP while sleep, reset or disabled!\n";
		return;
	}
	// In  only step on rising pulse
	if (value==0)
	{
		return;
	}

    if (m_bDir)
	{
        m_iCurStep-=m_uiStepSize;
	}
    else
	{
        m_iCurStep+=m_uiStepSize;
	}

    if (!m_cfg.bHasNoEndStops)
    {
		CheckEndstops();
    }

    m_fCurPos = StepToPos(m_iCurStep);
    uint32_t posOut;
	std::memcpy(&posOut, &m_fCurPos, sizeof(posOut)); // both 32 bits, just mangle it for sending over the wire.
    RaiseIRQ(POSITION_OUT, posOut);
}

void A4982::CheckEndstops()
{
	if (m_iCurStep<0)
	{
		m_iCurStep = 0;
		RaiseIRQ(MIN_OUT,1);
	}
	else if (m_iCurStep>m_iMaxPos)
	{
		m_iCurStep = m_iMaxPos;
		RaiseIRQ(MAX_OUT,1);
	}
	else
	{
		RaiseIRQ(MAX_OUT,0);
		RaiseIRQ(MIN_OUT,0);
	}
}

void A4982::ReparseConfig()
{
    m_iCurStep = PosToStep(m_cfg.fStartPos);
    m_iMaxPos = PosToStep(m_cfg.iMaxMM);
    m_fCurPos = m_cfg.fStartPos;
	m_fEnd = StepToPos(m_iMaxPos);
	CheckEndstops();
}

// Called when DRV_EN is triggered.
void A4982::OnEnableIn(struct avr_irq_t * /*irq*/, uint32_t value)
{
	TRACE(std::cout << "A4982 " << m_cAxis.load() << " ENABLE:" << m_bEnable << '\n');
    m_bEnable = value==0; // active low, i.e motors off when high.
}

A4982::A4982(char cAxis):m_cAxis(cAxis)
{
	m_strName.push_back(cAxis);
}

void A4982::OnResetIn(avr_irq_t */*irq*/, uint32_t value)
{
	m_bReset = value==0;
	// This also alters the motor waveforms to a home default
	// so that's a TODO, if we need to simulate to that level.
}

void A4982::OnSleepIn(avr_irq_t */*irq*/, uint32_t value)
{
	if (value)
	{
		m_bSleep = true;
	}
	else
	{
		// per datasheet, wake 1ms after line goes low.
		RegisterTimerUsec(m_fcnWakeup,1000,this);
	}
}

avr_cycle_count_t A4982::OnWakeup(struct avr_t *,avr_cycle_count_t)
{
	m_bSleep = false;
	return 0;
}

void A4982::OnMSIn(avr_irq_t *irq, uint32_t value)
{
	uint8_t uiM1 = 0, uiM2 = 0;
	if (irq== GetIRQ(MS1_IN))
	{
		 uiM1 = value;
		 uiM2 = GetIRQ(MS2_IN)->value;
	}
	else
	{
		uiM1 = GetIRQ(MS1_IN)->value;
		uiM2 = value;
	}
	uint8_t m_uiNewShift = (static_cast<unsigned>(uiM2)<<1U | static_cast<unsigned>(uiM1));
	std::cout << m_cAxis << " MS changed: " << std::to_string(m_uiNewShift) << '\n';
	switch (m_uiNewShift)
	{
		case 0:
			m_uiStepSize = 16; // Full step
			// Bitshift the position, the real driver will lose steps if you
			// change the step size while it's not at a common value...
			m_iCurStep = (static_cast<unsigned>(m_iCurStep) >> 4U) << 4U;
			break;
		case 1:
			m_uiStepSize = 8; // half-step
			m_iCurStep = (static_cast<unsigned>(m_iCurStep) >> 3U) << 3U;
			break;
		case 2:
			m_uiStepSize = 4;
			m_iCurStep = (static_cast<unsigned>(m_iCurStep) >> 2U) << 2U;
			break;
		case 3:
			m_uiStepSize = 1;
			// No shift, smallest supported microstep size.
			break;
		default: // pragma: LCOV_EXCL_LINE
			std::cerr << "A4982: PROGRAMMER ERROR! Invalid step size?\n"; // pragma: LCOV_EXCL_LINE
	}

}

void A4982::Init(struct avr_t * avr)
{
    _Init(avr, this);
	m_bConnected = true;

	ReparseConfig();

	GetIRQ(MIN_OUT)->flags |= IRQ_FLAG_FILTERED; // Don't re-raise if already at value.
	GetIRQ(MAX_OUT)->flags |= IRQ_FLAG_FILTERED; // Don't re-raise if already at value.
	GetIRQ(DIR_IN)->flags  |= IRQ_FLAG_FILTERED;
	GetIRQ(ENABLE_IN)->flags  |= IRQ_FLAG_FILTERED;
	GetIRQ(SLEEP_IN)->flags  |= IRQ_FLAG_FILTERED;
	GetIRQ(RESET_IN)->flags  |= IRQ_FLAG_FILTERED;

    RegisterNotify(DIR_IN,      MAKE_C_CALLBACK(A4982,OnDirIn), this);
    RegisterNotify(ENABLE_IN,   MAKE_C_CALLBACK(A4982,OnEnableIn), this);
	RegisterNotify(SLEEP_IN, 	MAKE_C_CALLBACK(A4982, OnSleepIn), this);
    RegisterNotify(STEP_IN,     MAKE_C_CALLBACK(A4982,OnStepIn), this);
	RegisterNotify(RESET_IN, 	MAKE_C_CALLBACK(A4982, OnResetIn), this);
	RegisterNotify(MS1_IN, MAKE_C_CALLBACK(A4982, OnMSIn), this);
	RegisterNotify(MS2_IN, MAKE_C_CALLBACK(A4982, OnMSIn), this);

	RaiseIRQ(MIN_OUT,0);
	RaiseIRQ(MAX_OUT,0);

	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, STEP_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, DIR_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, ENABLE_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, MS1_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, MS2_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, RESET_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, SLEEP_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, MAX_OUT,{TC::InputPin, TC::Stepper});
	TH.AddTrace(this, MIN_OUT,{TC::InputPin, TC::Stepper});
}

float A4982::StepToPos(int32_t step)
{
	// Position is always in 16ths of a step.
	return static_cast<float>(step)/static_cast<float>(m_cfg.uiStepsPerMM);
}

int32_t A4982::PosToStep(float pos)
{
	// Position is always 16ths of a step...
	return pos*static_cast<float>(m_cfg.uiStepsPerMM);
}
