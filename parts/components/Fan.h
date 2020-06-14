/*
    Fan.h Simple fan tach sim for Einsy Rambo

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
#pragma once

#include "BasePeripheral.h"
#include "Scriptable.h"

class Fan: public BasePeripheral, public Scriptable
{

public:
    // Macro to define a set of IRQs and string names.
    #define IRQPAIRS    _IRQ(PWM_IN,"<Fan.pwm_in") \
                        _IRQ(DIGITAL_IN, "<Fan.digital_in>")\
                        _IRQ(TACH_OUT, ">Fan.tach_out")\
                        _IRQ(SPEED_OUT, ">Fan.speed_out")

    // Helper to keep pairs in sync.
    #include <IRQHelper.h>

	// Constructs a new Fan with a max RPM of iMaxRPM (at PWM 255)
	Fan(uint16_t iMaxRPM);

	// Initializes the fan with avr, and connects to irqTach (out), irqDigital (in), and irqPWM (pwm control value)
	void Init(struct avr_t* avr, avr_irq_t *irqTach, avr_irq_t *irqDigital, avr_irq_t *irqPWM);

	// Flags the fan as stalled/jammed. or not.
	void SetStall(bool bStall);

	// Sets the RPM to a given value. Overrides auto control.
	void Set(uint16_t iRPM);

	// Clears an explicitly set RPM value and returns to automatic RPM calc.
	void Resume_Auto();

	protected:
			LineStatus ProcessAction(unsigned int ID, const vector<string> &vArgs) override;

	private:
		// Callback for tach pulse update.
		avr_cycle_count_t OnTachChange(avr_t *avr, avr_cycle_count_t when);

		// callback for PWM change.
		void OnPWMChange(avr_irq_t *irq, uint32_t value);

		// Callback for full on/off
		void OnDigitalChange(avr_irq_t *irq, uint32_t value);

		bool m_bAuto = true;
		bool m_bPulseState = false;
		uint8_t m_uiPWM = 0;
		uint16_t m_uiMaxRPM = 2000;
		uint16_t m_uiCurrentRPM = 0;
		uint16_t m_uiUsecPulse = 0;

		avr_cycle_timer_t m_fcnTachChange = MAKE_C_TIMER_CALLBACK(Fan,OnTachChange);

		enum Actions
		{
			Stall,
			Resume
		};

};
