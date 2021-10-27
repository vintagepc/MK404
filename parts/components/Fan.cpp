/*
    Fan.cpp - Simple fan tach sim for Einsy Rambo

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
#include "Fan.h"
#include "TelemetryHost.h"    // for TC, TelCategory, TelemetryHost
#include <algorithm>        // for copy
#include <iostream>

//#define TRACE(_w)_w
#ifndef TRACE
#define TRACE(_w)
#endif

Fan::Fan(uint16_t iMaxRPM, char chrSym, bool bIsSoftPWM):SoftPWMable(bIsSoftPWM,this),Scriptable("Fan"),GLIndicator(chrSym,false,true),m_bIsSoftPWM(bIsSoftPWM),m_uiMaxRPM(iMaxRPM)
{
	RegisterActionAndMenu("Stall", "Stalls the fan", Actions::Stall);
	RegisterActionAndMenu("Resume","Resumes fan from a stall condition",Actions::Resume);
	RegisterAction("SetPWM", "Sets the PWM to a specific value (0-255)",Actions::SetPWM,{ArgType::Int});
}

avr_cycle_count_t Fan::OnTachChange(avr_t *, avr_cycle_count_t)
{
    RaiseIRQ(TACH_OUT, m_bPulseState^=1);
    RegisterTimerUsec(m_fcnTachChange,m_uiUsecPulse,this);
	auto uiRot = RotateStep(21U);
	RaiseIRQ(ROTATION_OUT,uiRot);
    return 0;
}

avr_cycle_count_t Fan::OnFanDigiDelay(avr_t *, avr_cycle_count_t)
{
	OnPWMChange(nullptr, 255U*m_bDigiDelayVal);
    return 0;
}

Scriptable::LineStatus Fan::ProcessAction(unsigned int ID, const std::vector<std::string>& vArgs)
{
	switch (ID)
	{
		case Actions::SetPWM:
		{
			uint16_t uiRPM = (std::stoul(vArgs.at(0))*m_uiMaxRPM)/255U;
			std::cout << "New RPM: " << uiRPM << '\n';
			Set(uiRPM);
			return LineStatus::Finished;
		}
		case Actions::Stall:
			Set(0);
			return LineStatus::Finished;
		case Actions::Resume:
			Resume_Auto();
			return LineStatus::Finished;
	}
	return LineStatus::Unhandled;
}

void Fan::OnPWMChange(struct avr_irq_t*, uint32_t value)
{
    m_uiPWM = value;
    if (m_bAuto) // Only update RPM if auto (pwm-controlled). Else user supplied RPM.
	{
        m_uiCurrentRPM = ((m_uiMaxRPM)*value)/255;
	}
	SetValue(value>>1);
    RaiseIRQ(SPEED_OUT,m_uiCurrentRPM);

    float fSecPerRev = 60.0f/static_cast<float>(m_uiCurrentRPM);
    float fuSPerRev = 1000000*fSecPerRev;
    m_uiUsecPulse = fuSPerRev/4; // 4 pulses per rev.
    TRACE(printf("New PWM(%u)/RPM/cyc: %u / %u / %u\n", m_uiMaxRPM, m_uiPWM.load(), m_uiCurrentRPM, m_uiUsecPulse));
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
void Fan::OnDigitalChange(struct avr_irq_t *, uint32_t value)
{
	RaiseIRQ(PWM_IN, value*0xFFU);
}

void Fan::OnEnableInput(struct avr_irq_t* /*irq*/, uint32_t /*value*/)
{
	// Trigger update if PWM val or digital changes.
	OnEnableChange(GetIRQ(ENABLE_IN), GetIRQ(ENABLE_IN)->value);
}

void Fan::OnEnableChange(avr_irq_t* /*irq*/, uint32_t value)
{
	// printf("Enable %c changed: EN %02x DIG %02x PWM %02x\n", GetLabel(), value, GetIRQ(DIGITAL_IN)->value, GetIRQ(PWM_IN)->value);
	bool bSetTimer = false;
	if (value) {
	 	if (GetIRQ(DIGITAL_IN)->value) // 255 is a digitalwrite(1)
		{
			bSetTimer = true;
			value = 0xFFU;
		}
		else if(GetIRQ(PWM_IN)->value) // 1-254
		{
			value = GetIRQ(PWM_IN)->value;
		}
		else
		{
			bSetTimer = true;
			value = 0U;
		}
	}
	else // enable line low, fan off.
	{
		value = 0;

	}
	if (bSetTimer)
	{
		m_bDigiDelayVal = value>0;
 		RegisterTimerUsec(m_fcnFanDelay,5000,this);
	}
	else
	{
		CancelTimer(m_fcnFanDelay,this);
		if (value != m_uiPWM)
		{
			OnPWMChange(nullptr, value);
		}
	}
}

void Fan::Init(struct avr_t *avr, avr_irq_t *irqTach, avr_irq_t *irqDigital, avr_irq_t *irqPWM, bool bIsEnableCtl)
{
    _Init(avr, this);

    if(irqPWM)  ConnectFrom(irqPWM, PWM_IN);
    if(irqDigital) ConnectFrom(irqDigital, DIGITAL_IN);
    if(irqTach) ConnectTo(TACH_OUT,irqTach);


	// Note - this is partly for the CW1 FW where the enable pin is changed after the CWS so
	// we use that as a trigger to "lock in" the current value rather than trying to mess with the mashup of
	// digital and pwm signal.
	if (!bIsEnableCtl)
	{
    	RegisterNotify(PWM_IN, MAKE_C_CALLBACK(Fan,OnPWMChange), this);
		RegisterNotify(DIGITAL_IN,MAKE_C_CALLBACK(Fan,OnDigitalInSPWM), this);
	}
	else
	{
		RegisterNotify(PWM_IN, MAKE_C_CALLBACK(Fan,OnEnableInput), this);
		RegisterNotify(DIGITAL_IN, MAKE_C_CALLBACK(Fan,OnEnableInput), this);
	}
	RegisterNotify(ENABLE_IN,MAKE_C_CALLBACK(Fan,OnEnableChange), this);

	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, PWM_IN,{TC::PWM, TC::Fan},8);
	TH.AddTrace(this, DIGITAL_IN, {TC::Fan, TC::OutputPin});
	TH.AddTrace(this, TACH_OUT, {TC::Fan, TC::InputPin});
	TH.AddTrace(this, SPEED_OUT,{TC::Fan, TC::Misc},16);
	SetVisible(true);

}

void Fan::Set(uint16_t iRPM)
{
    m_bAuto = false;
    m_uiCurrentRPM = iRPM;
    RaiseIRQ(PWM_IN,m_uiPWM);
}

void Fan::Resume_Auto()
{
    m_bAuto = true;
	RaiseIRQ(PWM_IN,m_uiPWM);
}
