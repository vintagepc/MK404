/*
	Beeper.h - Beeper visualizer for MK404

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

#pragma once

#include "GLIndicator.h"
#include "IKeyClient.h"
#include "IScriptable.h"    // for IScriptable::LineStatus
#include "Scriptable.h"     // for Scriptable
#include "SoftPWMable.h"    // for SoftPWMable
#include "sim_avr.h"        // for avr_t
#include <SDL_audio.h>      // for SDL_AudioSpec
#include <atomic>
#include <cstdint>         // for uint16_t, uint8_t, uint32_t
#include <string>           // for string
#include <vector>           // for vector

class Beeper:public SoftPWMable, public Scriptable, private IKeyClient, public GLIndicator
{
	public:
		#define IRQPAIRS _IRQ(DIGITAL_IN,"<digital.in") _IRQ(PWM_IN,"<pwm.in")
		#include "IRQHelper.h"

		Beeper();
		~Beeper() override;
		// Initializes the LED to the AVR
		void Init(avr_t * avr);

	protected:
		void OnWaveformChange(uint32_t uiTOn,uint32_t uiTTotal) override;

		Scriptable::LineStatus ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs) override;

		void OnKeyPress(const Key &key) override;

	private:
		void StartTone();
		void SDL_FillBuffer(uint8_t *raw_buffer, int bytes);
		void UpdateMute(bool bVal);

		void(*m_fcnSDL)(void* p, uint8_t*, int) = [](void *p, uint8_t *raw_buffer, int bytes){auto self = static_cast<Beeper*>(p); self->SDL_FillBuffer(raw_buffer,bytes);};

		std::atomic_bool m_bPlaying = {false}, m_bMuted = {false};

		SDL_AudioSpec m_specWant {}, m_specHave {};

		std::atomic_uint16_t m_uiCtOn = {0}, m_uiCtOff = {0};
		std::atomic_uint16_t m_uiCounter {0};
		static constexpr uint16_t m_uiSampleRate = 44100;
		bool m_bState = false;
		bool m_bAudioAvail = false;
		enum Actions
		{
			ActMute,
			ActUnmute,
			ActToggle
		};
};
