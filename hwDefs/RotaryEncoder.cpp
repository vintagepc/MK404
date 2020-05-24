/*
    RotaryEncoder.cpp

    Based on simavr rotenc.c, Copyright 2018 Doug Szumski <d.s.szumski@gmail.com>

	Rewritten for C++ in 2020 by VintagePC <https://github.com/vintagepc/>

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

#include <stdio.h>

#include "RotaryEncoder.h"

static constexpr uint8_t  STATE_COUNT = 4;
static constexpr uint32_t PULSE_DURATION_US = 50000UL;
static constexpr uint32_t BUTTON_DURATION_US = 100000UL;
static constexpr uint32_t BUTTON_DURATION_LONG_US = 3000000UL; // 3s

static constexpr uint8_t m_States[STATE_COUNT] = {
	0b00,
	0b10,
	0b11,
	0b01
};

avr_cycle_count_t RotaryEncoder::OnStateChangeTimer(avr_t * avr,avr_cycle_count_t when)
{
	switch (m_eDirection) {
		case CW_CLICK:
			// Advance phase forwards
            m_iPhase = (++m_iPhase)%STATE_COUNT;

			if (m_bVerbose) {
				printf("ROTENC: CW twist, pins A:%x, B:%x\n",
					m_States[m_iPhase]>>1,
					m_States[m_iPhase]&1);
			}
			break;
		case CCW_CLICK:
			// Advance phase backwards
			 m_iPhase = (m_iPhase+3)%STATE_COUNT;
			if (m_bVerbose) {
				printf("ROTENC: CCW twist, pins: A:%x, B:%x\n",
					m_States[m_iPhase]>>1,
					m_States[m_iPhase]&1);
			}
			break;

		default:
			// Invalid direction
			break;
	}
    RaiseIRQ(OUT_A, m_States[m_iPhase]>>1);
    RaiseIRQ(OUT_B, m_States[m_iPhase]&1);
	
    if(--m_uiPulseCt >0) // Continue ticking the encoder
	{
		RegisterTimerUsec(m_fcnStateChange,PULSE_DURATION_US,this);
	}
	return 0;
}

avr_cycle_count_t RotaryEncoder::OnButtonReleaseTimer(avr_t * avr, avr_cycle_count_t when)
{
	RaiseIRQ(OUT_BUTTON, 1);
	if (m_bVerbose) {
		printf("ROTENC: Button release\n");
	}
	return 0;
}

void RotaryEncoder::_Push(uint32_t uiDuration)
{
	// Press down
	if (m_bVerbose) {
		printf("ROTENC: Button press\n");
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

/*
 * Simulates one "twist" pulse of the rotary encoder.
 */
void RotaryEncoder::Twist(Direction eDir)
{
	if (m_eDirection == eDir)
		m_uiPulseCt+=4; // Just tick it more if the dir is correct.
	else
	{
		m_eDirection = eDir;
		m_uiPulseCt = 4;
	}
	RegisterTimerUsec(m_fcnStateChange,PULSE_DURATION_US, this);
}

void RotaryEncoder::Init(avr_t *avr)
{
	_Init(avr, this);
}

