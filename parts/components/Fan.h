/*
    Fan.h Simple fan tach sim for Einsy Rambo

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

#include "BasePeripheral.h"    // for MAKE_C_TIMER_CALLBACK
#include "GLIndicator.h"
#include "IScriptable.h"       // for IScriptable::LineStatus
#include "Scriptable.h"        // for Scriptable
#include "SoftPWMable.h"       // for SoftPWMable
#include "sim_avr.h"           // for avr_t
#include "sim_avr_types.h"     // for avr_cycle_count_t
#include "sim_cycle_timers.h"  // for avr_cycle_timer_t
#include "sim_irq.h"           // for avr_irq_t
#include <cstdint>            // for uint16_t, uint32_t, uint8_t
#include <string>              // for string
#include <vector>              // for vector

class Fan:public SoftPWMable, public Scriptable, public GLIndicator
{

public:
    // Macro to define a set of IRQs and string names.
    #define IRQPAIRS    _IRQ(PWM_IN,"<Fan.pwm_in") \
						_IRQ(ENABLE_IN,"<Fan.enable_in") \
                        _IRQ(DIGITAL_IN, "<Fan.digital_in") \
						_IRQ(SPWM_IN, "<Fan.spwm_in")\
                        _IRQ(TACH_OUT, ">Fan.tach_out")\
                        _IRQ(SPEED_OUT, ">Fan.speed_out")\
						_IRQ(ROTATION_OUT, ">Fan.angle_out")

    // Helper to keep pairs in sync.
    #include "IRQHelper.h"

	// Constructs a new Fan with a max RPM of iMaxRPM (at PWM 255)
	explicit Fan(uint16_t iMaxRPM, char chrSym = ' ', bool bIsSoftPWM = false);

	// Initializes the fan with avr, and connects to irqTach (out), irqDigital (in), irqPWM (pwm control value), isEnableCtl (whether the fan is controlled with an EN line)
	void Init(struct avr_t* avr, avr_irq_t *irqTach, avr_irq_t *irqDigital, avr_irq_t *irqPWM, bool bIsEnableCt = false);

	// Flags the fan as stalled/jammed. or not.
	void SetStall(bool bStall);

	// Sets the RPM to a given value. Overrides auto control.
	void Set(uint16_t iRPM);

	// Clears an explicitly set RPM value and returns to automatic RPM calc.
	void Resume_Auto();

	protected:

		LineStatus ProcessAction(unsigned int ID, const std::vector<std::string> &vArgs) override;

		// callback for PWM change.
		void OnPWMChange(avr_irq_t *irq, uint32_t value) override;

		// Callback for full on/off
		void OnDigitalChange(avr_irq_t *irq, uint32_t value) override;

		void OnEnableChange(avr_irq_t *irq, uint32_t value);
		void OnEnableInput(struct avr_irq_t *, uint32_t value);

	private:
		// Callback for tach pulse update.
		avr_cycle_count_t OnTachChange(avr_t *avr, avr_cycle_count_t when);

		avr_cycle_count_t OnFanDigiDelay(avr_t *avr, avr_cycle_count_t when);

		bool m_bAuto = true;
		bool m_bPulseState = false;
		bool m_bIsSoftPWM = false;

		bool m_bDigiDelayVal = false;

		uint8_t m_uiPWM = 0;
		uint16_t m_uiMaxRPM = 2000;
		uint16_t m_uiCurrentRPM = 0;
		uint16_t m_uiUsecPulse = 0;

		avr_cycle_timer_t m_fcnTachChange = MAKE_C_TIMER_CALLBACK(Fan,OnTachChange);

		// Bit of an ugly hack for the bug that ODR writes update all bits in simavr...
		avr_cycle_timer_t m_fcnFanDelay = MAKE_C_TIMER_CALLBACK(Fan,OnFanDigiDelay);

		enum Actions
		{
			Stall,
			Resume,
			SetPWM
		};

};
