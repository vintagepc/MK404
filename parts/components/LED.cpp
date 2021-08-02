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


#include "LED.h"
#include "gsl-lite.hpp"
#include <GL/freeglut_std.h>          // for glutStrokeCharacter, GLUT_STROKE_MONO_R...
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glVertex2f, glBegin, glColor3f, glEnd
#else
# include <GL/gl.h>           // for glVertex2f, glBegin, glColor3f, glEnd
#endif

LED::LED(uint32_t uiHexColor, char chrLabel, bool bInvert):GLIndicator(chrLabel,bInvert)
{
    SetColor(uiHexColor);
}


void LED::OnValueChanged(struct avr_irq_t*, uint32_t value)
{
	SetValue(value*255U);
}

void LED::OnPWMChanged(struct avr_irq_t*, uint32_t value)
{
	SetValue(gsl::narrow<uint8_t>(value));
}

void LED::Init(avr_t *avr)
{
    _Init(avr, this);
    RegisterNotify(LED_IN,MAKE_C_CALLBACK(LED,OnValueChanged),this);
	RegisterNotify(PWM_IN,MAKE_C_CALLBACK(LED,OnPWMChanged),this);
	SetVisible(true);
}
