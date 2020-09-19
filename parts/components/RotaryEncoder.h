/*
    RotaryEncoder.h

    Based on simavr rotenc.h, Copyright 2018 Doug Szumski <d.s.szumski@gmail.com>

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


#pragma once

#include "BasePeripheral.h"    // for MAKE_C_TIMER_CALLBACK, BasePeripheral
#include "IKeyClient.h"
#include "IScriptable.h"       // for IScriptable::LineStatus
#include "Scriptable.h"        // for Scriptable
#include "sim_avr.h"           // for avr_t
#include "sim_avr_types.h"     // for avr_cycle_count_t
#include "sim_cycle_timers.h"  // for avr_cycle_timer_t
#include <cstdint>            // for uint32_t, uint8_t
#include <string>              // for string
#include <vector>              // for vector

class RotaryEncoder:public BasePeripheral,public Scriptable, private IKeyClient
{
    public:
        #define IRQPAIRS _IRQ(OUT_A,">encoder.a") _IRQ(OUT_B,">encoder.b") _IRQ(OUT_BUTTON,">encoder.button")
        #include "IRQHelper.h"

        using Direction = enum {
            CW_CLICK = 0,
            CCW_CLICK
        };

		explicit RotaryEncoder(bool bVerbose = false);

        // Registers a rotary encoder with "avr"
        void Init(avr_t *avr);

        // Twists the encoder in the direction "eDir"
        void Twist(Direction eDir);

        // Pushes and releases the button after a short delay.
        void Push();

        // Pushes and releases the button after an extended delay.
        void PushAndHold();

        // Does press event without a release for mouse control.
        void MousePush();

        // Release event for mouse push.
        void Release();
	protected:
		LineStatus ProcessAction(unsigned int action, const std::vector<std::string> &vArgs) override;

		void OnKeyPress(const Key &key) override;

    private:

        void _Push(uint32_t uiDuration);
        avr_cycle_count_t OnStateChangeTimer(avr_t * avr,avr_cycle_count_t when);

        avr_cycle_timer_t m_fcnStateChange = MAKE_C_TIMER_CALLBACK(RotaryEncoder,OnStateChangeTimer);

        avr_cycle_count_t OnButtonReleaseTimer(avr_t * avr,avr_cycle_count_t when);

        avr_cycle_timer_t m_fcnRelease = MAKE_C_TIMER_CALLBACK(RotaryEncoder,OnButtonReleaseTimer);

		enum Actions
		{
			ActTwistCW,
			ActTwistCCW,
			ActPress,
			ActRelease,
			ActPressAndRelease
		};

        bool m_bVerbose = false;
        uint8_t m_uiPulseCt = 0;
        Direction m_eDirection = CCW_CLICK;
        int m_iPhase = 0;			// current position
        bool m_bTimerRunning = false;

};
