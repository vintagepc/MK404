/*
	Button.cpp - simple button for SimAVR

	Original Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

	Rewritten/converted to c++ in 2020 by VintagePC <https://github.com/vintagepc/>

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

#include "Button.h"
#include "TelemetryHost.h"
#include <iostream>  // for printf
#include <memory>

Button::Button(const std::string &strName):Scriptable(strName),m_strName(strName)
{
	RegisterActionAndMenu("Press", "Simulate pressing the button", Actions::ActPress);
	RegisterActionAndMenu("Release", "Simulate releasing the button", Actions::ActRelease);
	RegisterActionAndMenu("PressAndRelease", "Simulate pressing and then releasing  the button", Actions::ActPressAndRelease);
}

Scriptable::LineStatus Button::ProcessAction(unsigned int iAction, const vector<string>&)
{
	switch (iAction)
	{
	case ActPressAndRelease:
		Press();
		break;
	case ActPress:
		RaiseIRQ(BUTTON_OUT, 0);// press
		break;
	case ActRelease:
		RaiseIRQ(BUTTON_OUT,1);
		break;
	}
	return LineStatus::Finished;
}

void Button::Init(avr_t* avr)
{
	// if name was not provided, init uses the defaults
	const char * pName = m_strName.c_str();
	_Init(avr, this, &pName);

	auto pTH = TelemetryHost::GetHost();
	pTH->AddTrace(this, BUTTON_OUT,{TC::InputPin, TC::Misc});

}


avr_cycle_count_t Button::AutoRelease(avr_t *, avr_cycle_count_t uiWhen)
{
	RaiseIRQ(BUTTON_OUT,1);
	cout << "Button released after " << uiWhen << " uSec" << '\n';
	return 0;
}

void Button::Press(uint32_t uiUsec)
{
	CancelTimer(m_fcnRelease,this);
	RaiseIRQ(BUTTON_OUT, 0);// press
	// register the auto-release
	RegisterTimerUsec(m_fcnRelease,uiUsec, this);
}
