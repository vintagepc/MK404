/*
	LED.h - Simple LED visualizer.

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

#include <stdint.h>          // for uint32_t, uint8_t
#include <atomic>
#include "BasePeripheral.h"  // for BasePeripheral
#include "Util.h"            // for hexColor_t
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t

class LED: public BasePeripheral
{
public:
	#define IRQPAIRS _IRQ(LED_IN,"<LED.in") _IRQ(PWM_IN,"<pwm.in")
	#include "IRQHelper.h"

	// Creates a new LED with RGBA color (A ignored) uiHexColor and char label chrLabel
	LED(uint32_t uiHexColor = 0x00FF0000, char chrLabel = ' ', bool bInvert = false);

	// Initializes the LED to the AVR
	void Init(avr_t * avr);

	// Draws the LED
	void Draw();

private:
	// Value changed callback.
	void OnValueChanged(avr_irq_t *irq, uint32_t value);
	void OnPWMChanged(avr_irq_t *irq, uint32_t value);
	hexColor_t m_Color = hexColor_t(0x00FF0000);
	char m_chrLabel = ' ';
	std::atomic_uint8_t m_uiBrightness = {0};
	bool m_bInvert = false;

};
