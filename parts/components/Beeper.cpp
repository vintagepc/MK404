/*
	Beeper.cpp - Beeper visualizer for MK3Sim

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

#include "Beeper.h"
#include <GL/freeglut_std.h>  // for glutStrokeCharacter, GLUT_STROKE_MONO_R...
#include <GL/gl.h>            // for glVertex2f, glPopMatrix, glPushMatrix
#include <SDL.h>              // for SDL_Init, SDL_INIT_AUDIO
#include <SDL_audio.h>        // for SDL_PauseAudio, SDL_AudioSpec, SDL_Clos...
#include <SDL_error.h>        // for SDL_GetError
#include <SDL_stdinc.h>       // for Sint16
#include <stdio.h>            // for fprintf, printf, stderr
#include "BasePeripheral.h"   // for MAKE_C_CALLBACK
#include "TelemetryHost.h"

Beeper::Beeper():SoftPWMable(true,this, 1, 100), Scriptable("Beeper")
{
	if (SDL_Init(SDL_INIT_AUDIO)!=0)
		fprintf(stderr,"Failed to init SDL_Audio\n");

    m_specWant.freq = m_uiSampleRate; // number of samples per second
    m_specWant.format = AUDIO_S16SYS; // sample type (here: signed short i.e. 16 bit)
    m_specWant.channels = 1; // only one channel
    m_specWant.samples = 1024; // buffer-size
    m_specWant.callback = m_fcnSDL; // function SDL calls periodically to refill the buffer
    m_specWant.userdata = this; // counter, keeping track of current sample number

	RegisterActionAndMenu("Mute","Mutes the beeper", ActMute);
	RegisterActionAndMenu("Unmute","Unmutes the beeper", ActUnmute);
	RegisterActionAndMenu("ToggleMute","Toggles the beeper mute", ActToggle);

    if(SDL_OpenAudio(&m_specWant, &m_specHave) != 0)
	{
		fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
		return;
	}
    if(m_specWant.format != m_specHave.format)
	{
		printf("Failed to get the desired AudioSpec\n");
		return;
	}
	m_bAudioAvail = true;

}

Scriptable::LineStatus Beeper::ProcessAction(unsigned int iAct, const vector<string> &vArgs)
{
	switch (iAct)
	{
		case ActMute:
		case ActUnmute:
			m_bMuted = iAct==ActMute;
			return LineStatus::Finished;
		case ActToggle:
			ToggleMute();
			return LineStatus::Finished;
	}
	return LineStatus::Unhandled;
}

Beeper::~Beeper()
{
	SDL_CloseAudio();
}

void Beeper::SDL_FillBuffer(uint8_t *raw_buffer, int bytes)
{
    Sint16 *buffer = (Sint16*)raw_buffer;
    for(int i = 0; i < bytes/2; i++, m_uiCounter--)
    {
		if (m_uiCounter==0)
		{
			m_bState ^=1;
			m_uiCounter = m_uiSampleRate/ (m_bState? m_uiCtOn : m_uiCtOff );
		}
        buffer[i] = m_bState? 12000 : -12000;
    }
}

void Beeper::OnWaveformChange(uint32_t uiTOn,uint32_t uiTTotal)
{
	if (m_bMuted)
		return;
	//printf("Beeper debug: %u on, %u total\n", uiTOn,uiTTotal);
	if (uiTOn == 0)
	{
		if (m_bAudioAvail) SDL_PauseAudio(1);
		m_bPlaying = false;
	}
	else
	{
		m_uiCtOn = m_pAVR->frequency/uiTOn;
		m_uiCtOff = m_pAVR->frequency/(uiTTotal-uiTOn);
		//printf("Beep @ %u Hz, duty cycle %u %\n",m_pAVR->frequency/uiTTotal, (100*uiTOn)/uiTTotal);
		if (m_uiCtOn == 0)
			return;
		{
			if (m_bPlaying && m_bAudioAvail)
				SDL_PauseAudio(1);
			m_uiCounter = m_uiSampleRate/m_uiCtOn;
			m_bPlaying = true;
			if (m_bAudioAvail) SDL_PauseAudio(0);
		}

	}
};

void Beeper::Init(avr_t *avr)
{
    _Init(avr, this);
	Beeper::RegisterNotify(DIGITAL_IN,MAKE_C_CALLBACK(Beeper,OnDigitalInSPWM), this);

	TelemetryHost::GetHost()->AddTrace(this, DIGITAL_IN, {TC::OutputPin, TC::Misc},8);
}

void Beeper::Draw()
{
	uint16_t uiBrt = 255;//((m_uiFreq*9)/10)+25;
    glPushMatrix();
        if (m_bPlaying)
            glColor3us(255*uiBrt, 128*uiBrt, 0);
        else
            glColor3ub(25,12,0);

        glBegin(GL_QUADS);
            glVertex2f(0,10);
            glVertex2f(20,10);
            glVertex2f(20,0);
            glVertex2f(0,0);
        glEnd();
        glColor3f(!m_bPlaying,!m_bPlaying,!m_bPlaying);
        glTranslatef(4,7,-1);
        glScalef(0.1,-0.05,1);
		glPushMatrix();
        	glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,'T');
		glPopMatrix();
		if (m_bMuted)
			glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,'X');
    glPopMatrix();
}
