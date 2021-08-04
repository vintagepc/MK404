/*
	Beeper.cpp - Beeper visualizer for MK404

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

#include "Beeper.h"
#include "BasePeripheral.h"   // for MAKE_C_CALLBACK
#include "IKeyClient.h"
#include "TelemetryHost.h"
#include "gsl-lite.hpp"
#include <SDL.h>              // for SDL_Init, SDL_INIT_AUDIO
#include <SDL_audio.h>        // for SDL_PauseAudio, SDL_AudioSpec, SDL_Clos...
#include <SDL_error.h>        // for SDL_GetError
#include <SDL_stdinc.h>       // for Sint16
#include <cstring>
#include <iostream>
#include <iterator>

Beeper::Beeper():SoftPWMable(true,this, 1, 100), Scriptable("Beeper"), IKeyClient(), GLIndicator('T',false, true)
{
	SetColor(0xFF800000);
	if (SDL_Init(SDL_INIT_AUDIO)!=0)
	{
		std::cerr << "Failed to init SDL_Audio" << '\n';
	}

    m_specWant.freq = m_uiSampleRate; // number of samples per second
    m_specWant.format = AUDIO_S16SYS; // sample type (here: signed short i.e. 16 bit)
    m_specWant.channels = 1; // only one channel
    m_specWant.samples = 1024; // buffer-size
    m_specWant.callback = m_fcnSDL; // function SDL calls periodically to refill the buffer
    m_specWant.userdata = this; // counter, keeping track of current sample number

	RegisterActionAndMenu("Mute","Mutes the beeper", ActMute);
	RegisterActionAndMenu("Unmute","Unmutes the beeper", ActUnmute);
	RegisterActionAndMenu("ToggleMute","Toggles the beeper mute", ActToggle);

	RegisterKeyHandler('m',"Mutes buzzer audio tones");

    if(SDL_OpenAudio(&m_specWant, &m_specHave) != 0)
	{
		std::cerr << "Failed to open audio: " << SDL_GetError() << '\n';
		return;
	}
    if(m_specWant.format != m_specHave.format)
	{
		std::cerr << "Failed to get the desired AudioSpec" << '\n';
		return;
	}
	m_bAudioAvail = true;

}

void Beeper::OnKeyPress(const Key &key)
{
	switch (key)
	{
		case 'm':
			UpdateMute(!m_bMuted);
			break;
	}
}

Scriptable::LineStatus Beeper::ProcessAction(unsigned int iAct, const std::vector<std::string>&)
{
	switch (iAct)
	{
		case ActMute:
		case ActUnmute:
			m_bMuted = iAct==ActMute;
			return LineStatus::Finished;
		case ActToggle:
			UpdateMute(!m_bMuted);
			return LineStatus::Finished;
	}
	return LineStatus::Unhandled;
}

Beeper::~Beeper()
{
	SDL_CloseAudio();
}

void Beeper::UpdateMute(bool bMuted) {
	m_bMuted = bMuted;
	SetDisabled(m_bMuted);
}

void Beeper::SDL_FillBuffer(uint8_t *raw_buffer, int bytes)
{

	Sint16 in = m_bState? 12000 : -12000;
	uint8_t data[2];
	std::memcpy(&data,&in,2);

	gsl::span<uint8_t> buffer(raw_buffer,bytes);
    for(auto it = buffer.begin(); it != buffer.end(); it+=2, m_uiCounter--)
    {
		if (m_uiCounter==0)
		{
			m_bState ^=1;
			in = m_bState? 12000 : -12000;
			std::memcpy(&data,&in,2);
			m_uiCounter = m_uiSampleRate/ (m_bState? m_uiCtOn : m_uiCtOff );
		}
        *it = data[0];
		*(std::next(it)) = data[1];
    }
}

void Beeper::OnWaveformChange(uint32_t uiTOn,uint32_t uiTTotal)
{
	if (m_bMuted)
	{
		return;
	}
	//printf("Beeper debug: %u on, %u total\n", uiTOn,uiTTotal);
	if (uiTOn == 0)
	{
		if (m_bAudioAvail) SDL_PauseAudio(1);
		m_bPlaying = false;
		SetValue(25);
	}
	else
	{
		m_uiCtOn = m_pAVR->frequency/uiTOn;
		m_uiCtOff = m_pAVR->frequency/(uiTTotal-uiTOn);
		//printf("Beep @ %u Hz, duty cycle %u %\n",m_pAVR->frequency/uiTTotal, (100*uiTOn)/uiTTotal);
		if (m_uiCtOn == 0)
		{
			return;
		}
		{
			if (m_bPlaying && m_bAudioAvail)
			{
				SDL_PauseAudio(1);
			}
			m_uiCounter = m_uiSampleRate/m_uiCtOn;
			m_bPlaying = true;
			SetValue(255);
			if (m_bAudioAvail) SDL_PauseAudio(0);
		}

	}
};

void Beeper::Init(avr_t *avr)
{
    _Init(avr, this);
	Beeper::RegisterNotify(DIGITAL_IN,MAKE_C_CALLBACK(Beeper,OnDigitalInSPWM), this);

	TelemetryHost::GetHost().AddTrace(this, DIGITAL_IN, {TC::OutputPin, TC::Misc},8);
	SetVisible(true);
}
