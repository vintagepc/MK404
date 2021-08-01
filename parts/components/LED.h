/*
	LED.h - Simple LED visualizer.

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

#include "BasePeripheral.h"  // for BasePeripheral
#include "GLIndicator.h"
#include "Util.h"            // for hexColor_t
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t
#include <atomic>
#include <cstdint>          // for uint32_t, uint8_t

class LED: public BasePeripheral, public GLIndicator
{
public:
	#define IRQPAIRS _IRQ(LED_IN,"<LED.in") _IRQ(PWM_IN,"<pwm.in")
	#include "IRQHelper.h"

	// Creates a new LED with RGBA color (A ignored) uiHexColor and char label chrLabel
	LED(uint32_t uiHexColor, char chrLabel, bool bInvert = false);

	// Initializes the LED to the AVR
	void Init(avr_t * avr);

private:
	// Value changed callback.
	void OnValueChanged(avr_irq_t *irq, uint32_t value);
	void OnPWMChanged(avr_irq_t *irq, uint32_t value);

};
