/*
	HD44780GL.h

    Based on SimAVR (hd44780_glut.h) which is:
	Copyright Luki <humbell@ethz.ch>
	Copyright 2011 Michel Pollet <buserror@gmail.com>

    Rewritten 2020 VintagePC <https://github.com/vintagepc/>
        Rewritten for C++
        Extended with brightness
        Enabled CGRAM functionality
        Changed to baked-in ROM .h

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

#include <stdint.h>   // for uint32_t, uint8_t
#include <atomic>
#include "HD44780.h"  // for HD44780
#include "sim_avr.h"  // for avr_t
#include "sim_irq.h"  // for avr_irq_t

class HD44780GL:public HD44780
{
	public:
		// Registers with SimAVR (for brightness)
		void Init(avr_t *avr);

		// Draws the display with the given colours within the current GL context.
		void Draw(uint32_t background,
			uint32_t character,
			uint32_t text,
			uint32_t shadow,
			bool bMaterial = false);

	private:
		// Char draw helper.
		void GLPutChar(char c, uint32_t character, uint32_t text, uint32_t shadow, bool bMaterial = false);

		void OnBrightnessPWM(avr_irq_t *irq, uint32_t value);
		void OnBrightnessDigital(avr_irq_t *irq, uint32_t value);


		uint8_t m_uiCharW = 5, m_uiCharH = 8;
		std::atomic_uint8_t m_uiBrightness = {255};
		std::atomic_uint8_t m_uiPWM = {255};
};
