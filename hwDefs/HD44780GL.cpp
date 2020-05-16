/*
	HD44780GL.cpp

    Based on SimAVR (hd44780_glut.c) which is:
	Copyright Luki <humbell@ethz.ch>
	Copyright 2011 Michel Pollet <buserror@gmail.com>

    Rewritten 2020 VintagePC <https://github.com/vintagepc/>
        Rewritten for C++ 
        Extended with brightness
        Enabled CGRAM functionality
        Changed to baked-in ROM .h

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

#include "HD44780GL.h"

#if __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif

#include "hd44780_charROM.h"	// generated with gimp

static inline void
glColor32U(uint32_t color)
{
	glColor4f(
			(float)((color >> 24) & 0xff) / 255.0f,
			(float)((color >> 16) & 0xff) / 255.0f,
			(float)((color >> 8) & 0xff) / 255.0f,
			(float)((color) & 0xff) / 255.0f );
}

void HD44780GL::Init(avr_t *avr)
{
	HD44780::Init(avr);

	RegisterNotify(BRIGHTNESS_IN, MAKE_C_CALLBACK(HD44780GL, OnBrightnessDigital),this);
	RegisterNotify(BRIGHTNESS_PWM_IN, MAKE_C_CALLBACK(HD44780GL, OnBrightnessPWM),this);
}

void HD44780GL::OnBrightnessPWM(struct avr_irq_t * irq, uint32_t value)
{
	//printf("Brightness pin changed value: %u\n",value);
	m_uiPWM = m_uiBrightness = value;
	SetFlag(HD44780_FLAG_DIRTY,1);
}

void HD44780GL::OnBrightnessDigital(struct avr_irq_t * irq,	uint32_t value)
{
	avr_regbit_t rb = AVR_IO_REGBIT(0x90,7); // COM3A1
	if (avr_regbit_get(m_pAVR,rb)) // Restore PWM value if being PWM-driven again after a digitalwrite
	{
		m_uiBrightness = m_uiPWM;
		return;
	}
	//printf("Brightness digital pin changed: %02x\n",value);
	if (value)
		m_uiBrightness = 0xFF;
	else
		m_uiBrightness = 0x00;
	SetFlag(HD44780_FLAG_DIRTY,1);
	
}


void HD44780GL::GLPutChar(char c, uint32_t character, uint32_t text, uint32_t shadow)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor32U(character);
	glBegin(GL_QUADS);
	glVertex3i(5, 8, 0);
	glVertex3i(0, 8, 0);
	glVertex3i(0, 0, 0);
	glVertex3i(5, 0, 0);
	glEnd();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	uint8_t *uiData;
	uint8_t iCols=8;
	if (c<16)
		uiData = &m_cgRam[(c & 7) <<3];
	else
	{
		uiData = (uint8_t*)&hd44780_ROM_AOO.data[c*hd44780_ROM_AOO.h];
		iCols = 7;
	}
		
	for (int i=0; i < iCols; i++)
	{
		 TRACE(printf("%u%u%u%u%u\n", 
		 	(uiData[i] & 1)==1,
		 	(uiData[i] & 2)>1,
		 	(uiData[i] & 4)>1,
		 	(uiData[i] & 8)>1,
			(uiData[i] & 16)>1));
		for (int j=0; j<5; j++)
		{
			
			if (uiData[i] & (16>>j))
			{
				float x = (float)j;
				float y = (float)i;
				float inset = 0.85;
				if (shadow)
				{
					glPushMatrix();
					glColor32U(shadow);
					glVertex3f(x,y,0);
					glVertex3f(x+1,y,0);
					glVertex3f(x+1,y+1,0);
					glVertex3f(x,y+1,0);
					glPopMatrix();
				}
				glColor32U(text);
				glVertex3f(x,y,0);
				glVertex3f(x+inset,y,0);
				glVertex3f(x+inset,y+inset,0);
				glVertex3f(x,y+inset,0);

			}
		}
	}
	glEnd();
}

void HD44780GL::Draw(
		uint32_t background,
		uint32_t character,
		uint32_t text,
		uint32_t shadow)
{
	uint8_t iCols = m_uiWidth;
	uint8_t iRows = m_uiHeight;
	int border = 3;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	uint8_t* iBG = (uint8_t*)&background;
	float r = iBG[3]/255.0,g = iBG[2]/255.0,blue = iBG[1]/255.0;
	float fScale = (float)m_uiBrightness/255.0;
	glColor4f(r*fScale,g*fScale,blue*fScale,1.0f);
	glTranslatef(border, border, 0);
	glBegin(GL_QUADS);
	glVertex3f(iCols * m_uiCharW + (iCols - 1) + border, -border, 0);
	glVertex3f(-border, -border, 0);
	glVertex3f(-border, iRows * m_uiCharH + (iRows - 1) + border, 0);
	glVertex3f(iCols * m_uiCharW + (iCols - 1) + border, iRows * m_uiCharH
	        + (iRows - 1) + border, 0);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	// TODO: Something is not right with these offsets. The stock ones
	// (0, 0x20,0x40,0x60) were totally whack-o, these seem to kinda-sorta work but
	// there are still lingering display quirks where stuff renders offscreen. Probably the actual memory
	// mapping needs to change instead of just the offsets since I suspect there are still
	// corner cases that write into incorrect areas.
	uint8_t offset[] = {0, 0x40, iCols, static_cast<uint8_t>(iCols+0x40)};
	for (int v = 0 ; v < m_uiHeight; v++) {
		glPushMatrix();
		for (int i = 0; i < m_uiWidth; i++) {
			GLPutChar(m_vRam[offset[v] + i], character, text, shadow);
			glTranslatef(6, 0, 0);
		}
		glPopMatrix();
		glTranslatef(0, 9, 0);
	}
	//SetFlag(HD44780_FLAG_DIRTY, 0);
}
