/*
	Beeper.h - Beeper visualizer for MK3Sim

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

#include "SoftPWMable.h"

class Beeper:public SoftPWMable
{
	public:
		#define IRQPAIRS _IRQ(DIGITAL_IN,"<digital.in") _IRQ(PWM_IN,"<pwm.in")
		#include "IRQHelper.h"

		Beeper();

		// Initializes the LED to the AVR
		void Init(avr_t * avr);


		// Draws the LED
		void Draw();


	protected:
		virtual void OnDigitalChange(avr_irq_t*, uint32_t) override;
		virtual void OnPWMChange(avr_irq_t*, uint32_t) override;

	private:
		uint16_t m_uiFreq = 0; //
};
