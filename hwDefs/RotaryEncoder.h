/*
    RotaryEncoder.h

    Based on simavr rotenc.h, Copyright 2018 Doug Szumski <d.s.szumski@gmail.com>

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


#ifndef __ROTARYENCODER_H__
#define __ROTARYENCODER_H__

#include "BasePeripheral.h"

class RotaryEncoder:public BasePeripheral
{
    public:
        #define IRQPAIRS _IRQ(OUT_A,">encoder.a") _IRQ(OUT_B,">encoder.b") _IRQ(OUT_BUTTON,">encoder.button")
        #include "IRQHelper.h"

        typedef enum {
            CW_CLICK = 0,
            CCW_CLICK
        } Direction;

        // Registers a rotary encoder with "avr"
        void Init(avr_t *avr);

        // Twists the encoder in the direction "eDir"
        void Twist(Direction eDir);

        // Pushes and releases the button after a short delay.
        void Push();

        // Pushes and releases the button after an extended delay.
        void PushAndHold();

    private:

        void _Push(uint32_t uiDuration);
        avr_cycle_count_t OnStateChangeTimer(avr_t * avr,avr_cycle_count_t when);

        avr_cycle_count_t OnButtonReleaseTimer(avr_t * avr,avr_cycle_count_t when);

        bool m_bVerbose = false;
        uint8_t m_uiPulseCt = 0;
        Direction m_eDirection = CCW_CLICK;
        int m_iPhase = 0;			// current position

};

#endif /* __ROTENC_H__*/
