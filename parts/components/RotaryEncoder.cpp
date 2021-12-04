/*
    RotaryEncoder.cpp

    Based on simavr rotenc.c, Copyright 2018 Doug Szumski <d.s.szumski@gmail.com>

	Rewritten for C++ in 2020 by VintagePC <https://github.com/vintagepc/>

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

#include "KeyController.h"
#include "RotaryEncoder.h"
#include "TelemetryHost.h"
#include "gsl-lite.hpp"
#include <GL/glut.h>
#include <algorithm>         // for copy
#include <iostream>

static constexpr uint8_t  STATE_COUNT = 4;
static constexpr uint32_t PULSE_DURATION_US = 10000UL;
static constexpr uint32_t BUTTON_DURATION_US = 100000UL;
static constexpr uint32_t BUTTON_DURATION_LONG_US = 3000000UL; // 3s

static constexpr uint8_t m_States[STATE_COUNT] = {
	0b00,
	0b10,
	0b11,
	0b01
};

avr_cycle_count_t RotaryEncoder::OnStateChangeTimer(avr_t *,avr_cycle_count_t)
{
	if (m_eDirection == CW_CLICK)
	{
		// Advance phase forwards
		m_iPhase = (m_iPhase+1)%STATE_COUNT;

		if (m_bVerbose)
		{
			std::cout << "RotaryEncoder: CW Twist" << '\n';
		}
	}
	else
	{
		// Advance phase backwards
		m_iPhase = (m_iPhase+3)%STATE_COUNT;
		if (m_bVerbose)
		{
			std::cout << "RotaryEncoder: CCW twist" << '\n';
		}
	}
    RaiseIRQ(OUT_A, gsl::at(m_States, m_iPhase)>>1U);
    RaiseIRQ(OUT_B, gsl::at(m_States, m_iPhase)&1U);

    if(--m_uiPulseCt >0) // Continue ticking the encoder
	{
		RegisterTimerUsec(m_fcnStateChange,PULSE_DURATION_US,this);
	}
	else
	{
		m_bTimerRunning = false;
	}
	return 0;
}

avr_cycle_count_t RotaryEncoder::OnButtonReleaseTimer(avr_t *, avr_cycle_count_t)
{
	RaiseIRQ(OUT_BUTTON, 1);
	if (m_bVerbose)	std::cout << "ROTENC: Button release\n";

	return 0;
}

void RotaryEncoder::_Push(uint32_t uiDuration)
{
	// Press down
	if (m_bVerbose) {
		std::cout << "ROTENC: Button press\n";
	}
	RaiseIRQ(OUT_BUTTON, 0);

	// Pull up later
	RegisterTimerUsec(m_fcnRelease, uiDuration, this);
}

void RotaryEncoder::Push()
{
	_Push(BUTTON_DURATION_US);
}

void RotaryEncoder::PushAndHold()
{
	_Push(BUTTON_DURATION_LONG_US);
}

void RotaryEncoder::MousePush()
{
	RaiseIRQ(OUT_BUTTON, 0);
}

void RotaryEncoder::Release()
{
	RaiseIRQ(OUT_BUTTON, 1);
}

/*
 * Simulates one "twist" pulse of the rotary encoder.
 */
void RotaryEncoder::Twist(Direction eDir)
{
	Expects(eDir == CCW_CLICK || eDir == CW_CLICK);
	if (m_eDirection == eDir)
	{
		m_uiPulseCt+=4; // Just tick it more if the dir is correct.
	}
	else
	{
		m_eDirection = eDir;
		m_uiPulseCt = 4;
	}
	if (!m_bTimerRunning) // Don't register if the timer is already ticking.
	{
		m_bTimerRunning = true;
		RegisterTimerUsec(m_fcnStateChange,PULSE_DURATION_US, this);
	}
}

void RotaryEncoder::Init(avr_t *avr)
{
	_Init(avr, this);
	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this,OUT_BUTTON,{TC::InputPin, TC::Display});
	TH.AddTrace(this,OUT_A,{TC::InputPin, TC::Display});
	TH.AddTrace(this,OUT_B,{TC::InputPin, TC::Display});
}

Scriptable::LineStatus RotaryEncoder::ProcessAction(unsigned int iAct, const std::vector<std::string>&)
{
	switch (iAct)
	{
		case ActTwistCW:
			Twist(CW_CLICK);
			break;
		case ActTwistCCW:
			Twist(CCW_CLICK);
			break;
		case ActPressAndRelease:
			Push();
			break;
		case ActPress:
			MousePush();
			break;
		case ActRelease:
			Release();
			break;
	}
	return LineStatus::Finished;
}

void RotaryEncoder::OnKeyPress(const Key &key)
{
	switch (key)
	{
		case GLUT_KEY_UP | SPECIAL_KEY_MASK:
		case 'w':
			std::cout << '<';
			Twist(RotaryEncoder::CCW_CLICK);
			//if (m_pVis) m_pVis->TwistKnob(true);
			break;
		case GLUT_KEY_DOWN | SPECIAL_KEY_MASK:
		case 's':
			std::cout << '>';
			Twist(RotaryEncoder::CW_CLICK);
			//if (m_pVis) m_pVis->TwistKnob(false);
			break;
		case 0xd:
			std::cout << "ENTER pushed\n";
			Push();
			break;
		case 'h':
			PushAndHold();
			break;
	}
}

RotaryEncoder::RotaryEncoder(bool bVerbose):Scriptable("Encoder"),IKeyClient(),m_bVerbose{bVerbose}
{
	RegisterActionAndMenu("Press", "Presses the encoder button",ActPress);
	RegisterActionAndMenu("Release", "Releases the encoder button",ActRelease);
	RegisterActionAndMenu("PressAndRelease", "Presses the encoder button",ActPressAndRelease);
	RegisterActionAndMenu("TwistCW", "Twists the encoder one cycle clockwise",ActTwistCW);
	RegisterActionAndMenu("TwistCCW", "Twists the encoder once cycle counterclockwise",ActTwistCCW);

	RegisterKeyHandler('w', "Twists encoder CCW");
	RegisterKeyHandler(GLUT_KEY_UP | SPECIAL_KEY_MASK, "");
	RegisterKeyHandler('s', "Twists encoder CW");
	RegisterKeyHandler(GLUT_KEY_DOWN | SPECIAL_KEY_MASK, "");
	RegisterKeyHandler('h', "Pushes and long-holds the encoder button.");
	RegisterKeyHandler(0xd, "Pushes and releases the encoder button");
}
