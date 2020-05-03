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
 
#ifndef __HD44780_GL_H__
#define __HD44780_GL_H__

#include "HD44780.h"

class HD44780GL:public HD44780
{
	public:

		void Init(avr_t *avr);

		void Draw(uint32_t background,
			uint32_t character,
			uint32_t text,
			uint32_t shadow);

	private:
		void GLPutChar(char c, uint32_t character, uint32_t text, uint32_t shadow);

		void OnBrightnessPWM(avr_irq_t *irq, uint32_t value);
		void OnBrightnessDigital(avr_irq_t *irq, uint32_t value);


		uint8_t m_uiCharW = 5, m_uiCharH = 8;
		uint8_t m_uiBrightness = 255;
		uint8_t m_uiPWM = 255;
};
#endif
