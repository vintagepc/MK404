/*
	Button.cpp - simple button for SimAVR

	Original Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

	Rewritten/converted to c++ in 2020 by VintagePC <https://github.com/vintagepc/>

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

#include <stdlib.h>
#include <stdio.h>
#include "Button.h"

Button::Button(std::string strName):m_strName(strName)
{
}

void Button::Init(avr_t* avr)
{
	// if name was not provided, init uses the defaults
	const char * pName = m_strName.c_str();
	_Init(avr, this, &pName);
}


avr_cycle_count_t Button::AutoRelease(avr_t *avr, avr_cycle_count_t uiWhen)
{
	RaiseIRQ(BUTTON_OUT,1);
	printf("Button released after %lu usec\n",uiWhen);
	return 0;
}

void Button::Press(uint32_t uiUsec)
{
	CancelTimer(m_fcnRelease,this);
	RaiseIRQ(BUTTON_OUT, 0);// press
	// register the auto-release
	RegisterTimerUsec(m_fcnRelease,uiUsec, this);
}
