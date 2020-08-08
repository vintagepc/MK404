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

#pragma once

#include "BasePeripheral.h"

class SoftPWMable : public BasePeripheral
{
	public:
		template<class C>
		SoftPWMable(bool bEnabled, C *p, uint16_t uiPrescale = 1000, uint32_t uiTimeoutMs = 17):m_bIsSoftPWM(bEnabled),m_uiPrescale(uiPrescale)
		{
			m_uiSoftTimeoutUs = 1000*uiTimeoutMs;
			m_fcnSoftTimeout = MAKE_C_TIMER_CALLBACK(SoftPWMable,OnSoftPWMChangeTimeout<C>);
		};

	protected:

		// You will receive soft PWM values here.
		virtual void OnPWMChange(avr_irq_t *irq, uint32_t value){};

		// Override this with your normal method for a digital change when NOT soft-PWMed
		virtual void OnDigitalChange(avr_irq_t *irq, uint32_t value){};

		// Called when the frequency changes.
		virtual void OnWaveformChange(uint32_t uiTOn, uint32_t uiTTotal) {};

		// Binding for soft PWM digital input register notify.
		inline void OnDigitalInSPWM(avr_irq_t *irq, uint32_t value)
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

		// Callback for handling full on/off states with softPWM tracking.
		template<class C>
		avr_cycle_count_t OnSoftPWMChangeTimeout(avr_t *avr, avr_cycle_count_t when)
		{
			//printf("Timeout\n");
			OnPWMChange(GetIRQ(C::DIGITAL_IN), (GetIRQ(C::DIGITAL_IN)->value)*255);
			OnWaveformChange(0,0);
			m_cntTOn = 0;
			return 0;
		}

	private:

		uint32_t m_uiSoftTimeoutUs = 17 *1000; // 62.5 Hz = 16ms period max...

		bool m_bIsSoftPWM = false;

		uint16_t m_uiPrescale = 1000;
		avr_cycle_count_t m_cntSoftPWM = 0, m_cntTOn = 0;
		avr_cycle_timer_t m_fcnSoftTimeout;

};
