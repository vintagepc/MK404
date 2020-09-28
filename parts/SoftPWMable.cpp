/*
	SoftPWMable.h - Helper for soft-PWMed items that don't use PWM timers.

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

#include "SoftPWMable.h"


// Binding for soft PWM digital input register notify.
void SoftPWMable::OnDigitalInSPWM(avr_irq_t *irq, uint32_t value)
{
	if (!m_bIsSoftPWM) // For softpwm,
	{
		OnDigitalChange(irq,value);
		return;
	}
	if (value) // Was off, start at full, we'll update rate later.
	{
		RegisterTimerUsec(m_fcnSoftTimeout,m_uiSoftTimeoutUs,this);
		if (m_cntTOn>m_cntSoftPWM)
		{
			uint32_t uiTTotal = m_pAVR->cycle - m_cntSoftPWM;
			OnWaveformChange(m_cntTOn-m_cntSoftPWM,uiTTotal);
		}
		m_cntSoftPWM = m_pAVR->cycle;
	}
	else if (!value)
	{
		uint64_t uiCycleDelta = m_pAVR->cycle - m_cntSoftPWM;
		//TRACE(printf("New soft PWM delta: %d\n",uiCycleDelta/1000));
		uint16_t uiSoftPWM = ((uiCycleDelta/m_uiPrescale)-1); //62.5 Hz means full on is ~256k cycles.
		OnPWMChange(irq,uiSoftPWM);
		m_cntTOn = m_pAVR->cycle;
		RegisterTimerUsec(m_fcnSoftTimeout,m_uiSoftTimeoutUs,this);
	}
}
