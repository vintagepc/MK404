/*
	Beeper.h - Beeper visualizer for MK3Sim

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

#pragma once

#include <SDL/SDL_audio.h>  // for SDL_AudioSpec
#include <stdint.h>         // for uint16_t, uint8_t, uint32_t
#include <string>           // for string
#include <vector>           // for vector
#include "IScriptable.h"    // for IScriptable::LineStatus
#include "Scriptable.h"     // for Scriptable
#include "SoftPWMable.h"    // for SoftPWMable
#include "sim_avr.h"        // for avr_t

class Beeper:public SoftPWMable, public Scriptable
{
	public:
		#define IRQPAIRS _IRQ(DIGITAL_IN,"<digital.in") _IRQ(PWM_IN,"<pwm.in")
		#include "IRQHelper.h"

		Beeper();
		~Beeper();
		// Initializes the LED to the AVR
		void Init(avr_t * avr);

		// Draws the LED
		void Draw();

		inline void ToggleMute(){m_bMuted^=1;}


	protected:
		virtual void OnWaveformChange(uint32_t uiTOn,uint32_t uiTTotal) override;

		Scriptable::LineStatus ProcessAction(unsigned int iAct, const vector<string> &vArgs) override;

	private:
		void StartTone();
		void SDL_FillBuffer(uint8_t *raw_buffer, int bytes);

		void(*m_fcnSDL)(void* p, uint8_t*, int) = [](void *p, uint8_t *raw_buffer, int bytes){Beeper *self = static_cast<Beeper*>(p); self->SDL_FillBuffer(raw_buffer,bytes);};

		bool m_bPlaying = false;

		SDL_AudioSpec m_specWant, m_specHave;

		uint16_t m_uiCtOn = 0, m_uiCtOff = 0;
		uint16_t m_uiCounter = 0;
		static constexpr uint16_t m_uiSampleRate = 44100;
		bool m_bState = false, m_bMuted = false;

		enum Actions
		{
			ActMute,
			ActUnmute,
			ActToggle
		};
};
