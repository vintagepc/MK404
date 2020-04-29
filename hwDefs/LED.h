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


#ifndef __LED_H___
#define __LED_H___

#include "BasePeripheral.h"

class LED: public BasePeripheral 
{
public:
	#define IRQPAIRS _IRQ(LED_IN,"<LED.in")
	#include "IRQHelper.h"

	// Creates a new LED with RGBA color (A ignored) uiHexColor and char label chrLabel
	LED(uint32_t uiHexColor = 0x00FF0000, char chrLabel = ' ');

	// Initializes the LED to the AVR
	void Init(avr_t * avr);

	// Draws the LED
	void Draw();

private:
	// Value changed callback.
	void OnValueChanged(avr_irq_t *irq, uint32_t value);
	float m_fColor[3] = {0,1,0};
	char m_chrLabel = ' ';
	bool m_bOn = false;

};
#endif /* __LED_H___ */
