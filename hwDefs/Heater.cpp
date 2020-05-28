/*
	Heater.cpp - a heater object for MK3Sim. There's not much to it, 
    it just ticks the temperature "up" at a determined rate when active on PWM and down in
    in an exponential curve when off. 

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

#include "Heater.h"
#include "stdio.h"
#include "math.h"

//#define TRACE(_w)_w
#ifndef TRACE
#define TRACE(_w)
#endif


avr_cycle_count_t Heater::OnTempTick(avr_t * avr, avr_cycle_count_t when)
{
    if (m_uiPWM>0)
    {
        float fDelta = (m_fThermalMass*((float)(m_uiPWM)/255.0f))*0.3;
        m_fCurrentTemp += fDelta;
    }
    else // Cooling - do a little exponential decay
    {
        float dT = (m_fCurrentTemp - m_fAmbientTemp)*pow(2.7183,-0.005*0.3);
        m_fCurrentTemp -= m_fCurrentTemp - (m_fAmbientTemp + dT);
    }
        
    TRACE(printf("New temp value: %.02f\n",m_fCurrentTemp));
    RaiseIRQ(TEMP_OUT,(int)m_fCurrentTemp*256);

    if (m_uiPWM>0 || m_fCurrentTemp>m_fAmbientTemp+0.3)
        RegisterTimerUsec(m_fcnTempTick,300000,this);
    else
    {
        m_fCurrentTemp = m_fCurrentTemp;
        RaiseIRQ(TEMP_OUT,(int)m_fCurrentTemp*256);
    }
    return 0;
}


void Heater::OnPWMChanged(struct avr_irq_t * irq,uint32_t value)
{
    if (m_bAuto) // Only update if auto (pwm-controlled). Else user supplied RPM.
        m_uiPWM = value;

    if (m_uiPWM > 0)
        RegisterTimerUsec(m_fcnTempTick, 100000, this);
    if ((m_pIrq + ON_OUT)->value != (m_uiPWM>0))
        RaiseIRQ(ON_OUT,m_uiPWM>0);
}

//TCCR0A  _SFR_IO8(0x24)
//#define COM0B0  4

void Heater::OnDigitalChanged(struct avr_irq_t * irq, uint32_t value)
{

    if (m_bIsBed) // The heatbed PWM is based on inverting mode trickery. We can just watch COM0B0 rather than the digital pin output.
    {   
        avr_regbit_t inv = AVR_IO_REGBIT(0x24 + 32,4);
        uint8_t COM0B0 = avr_regbit_get(m_pAVR, inv);
        value = COM0B0^1;
    }

    if (value==1)
        value = 255;

    OnPWMChanged(irq,value);
}

Heater::Heater(float fThermalMass, float fAmbientTemp, bool bIsBed):
                                            m_fThermalMass(fThermalMass),
                                            m_fAmbientTemp(fAmbientTemp), 
                                            m_fCurrentTemp(fAmbientTemp),
                                            m_bIsBed(bIsBed)            
{

}

void Heater::Init(struct avr_t * avr, avr_irq_t *irqPWM,avr_irq_t *irqDigital)
{
    _Init(avr, this);
    if(irqPWM) ConnectFrom(irqPWM, PWM_IN);
    if(irqDigital) ConnectFrom(irqDigital, DIGITAL_IN);

    RegisterNotify(PWM_IN, MAKE_C_CALLBACK(Heater,OnPWMChanged),this);
    RegisterNotify(DIGITAL_IN, MAKE_C_CALLBACK(Heater,OnDigitalChanged),this);
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
}