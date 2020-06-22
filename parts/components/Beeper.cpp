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
#include <stdio.h>
#include <GL/glut.h>
#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>

Beeper::Beeper():SoftPWMable(true,this, 1, 75)
{
	if (SDL_Init(SDL_INIT_AUDIO)!=0)
		fprintf(stderr,"Failed to init SDL_Audio\n");

	printf("Starting to play tone of %d Hz\n",m_uiFreq);
    m_specWant.freq = m_uiSampleRate; // number of samples per second
    m_specWant.format = AUDIO_S16SYS; // sample type (here: signed short i.e. 16 bit)
    m_specWant.channels = 1; // only one channel
    m_specWant.samples = 1024; // buffer-size
    m_specWant.callback = m_fcnSDL; // function SDL calls periodically to refill the buffer
    m_specWant.userdata = this; // counter, keeping track of current sample number

    SDL_AudioSpec have;
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
}

Beeper::~Beeper()
{
}

void Beeper::SDL_FillBuffer(uint8_t *raw_buffer, int bytes)
{
    Sint16 *buffer = (Sint16*)raw_buffer;
    for(int i = 0; i < bytes/2; i++, m_uiCounter--)
    {
		if (m_uiCounter==0)
		{
			m_uiCounter = m_uiSampleRate/(m_uiPlayFreq<<1);
			m_bState ^=1;
		}
        buffer[i] = m_bState? 12000 : -12000;
    }
}

void Beeper::OnDigitalChange(avr_irq_t *irq, uint32_t value)
{
	//printf("Beeper turned on: %d\n", value);
}

void Beeper::OnPWMChange(avr_irq_t* irq, uint32_t value)
{
	m_uiPWM = value;
	//printf("Beeper PWM change: %d\n", value);
}

void Beeper::OnOnCycChange(uint32_t uiTOn)
{
	if (uiTOn == 0)
	{
		m_uiFreq = 0;
		m_uiOnTime = 0;
		SDL_PauseAudio(1);
		m_bPlaying = false;
	}
	else
	{
		m_uiOnTime = uiTOn<<2;
		//printf("TOn: %d\n",uiTOn);
		m_uiFreq = (m_pAVR->frequency/m_uiOnTime)<<1;
		{
			if (m_bPlaying)
				SDL_PauseAudio(1);
			printf("Beeper frequency: %d\n",m_uiFreq);
			m_uiPlayFreq = m_uiFreq;
			m_uiCounter = m_uiSampleRate/(m_uiPlayFreq<<1);
			m_bPlaying = true;
			SDL_PauseAudio(0);
		}

	}
};

void Beeper::Init(avr_t *avr)
{
    _Init(avr, this);
	Beeper::RegisterNotify(DIGITAL_IN,MAKE_C_CALLBACK(Beeper,OnDigitalInSPWM), this);
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
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,'T');
    glPopMatrix();
}
