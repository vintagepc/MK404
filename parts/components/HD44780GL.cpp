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

#include "Util.h"

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
glColorHelper(hexColor_t color, bool bMaterial = false)
{

	if (bMaterial)
	{
		float fCol[4] = {	(float)(color.red)/255.0f,
					(float)(color.green) / 255.0f,
					(float)(color.blue) / 255.0f,
					(float)(color.alpha) / 255.0f };
		float fNone[4] = {0,0,0,1};
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE | GL_SPECULAR, fNone);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  fCol);
	}
	else
		glColor4ub(color.red, color.green, color.blue, color.alpha);

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

void HD44780GL::GLPutChar(char c, uint32_t character, uint32_t text, uint32_t shadow, bool bMaterial)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColorHelper(character,bMaterial);
	glBegin(GL_QUADS);
		glVertex3i(5, 8, -1);
		glVertex3i(5, 0, -1);
		glVertex3i(0, 0, -1);
		glVertex3i(0, 8, -1);
	glEnd();
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
						glColorHelper(shadow, bMaterial);
						glBegin(GL_QUADS);
							glVertex3f(x,y,		-2);
							glVertex3f(x,y+1,	-2);
							glVertex3f(x+1,y+1,	-2);
							glVertex3f(x+1,y,	-2);
						glEnd();
					glPopMatrix();
				}
				glColorHelper(text, bMaterial);
				glBegin(GL_QUADS);
					glVertex3f(x,y,				-3);
					glVertex3f(x,y+inset,		-3);
					glVertex3f(x+inset,y+inset,	-3);
					glVertex3f(x+inset,y,		-3);
				glEnd();
			}
		}
	}
}


void HD44780GL::Draw(
		uint32_t background,
		uint32_t character,
		uint32_t text,
		uint32_t shadow, bool bMaterial)
{
	uint8_t iCols = m_uiWidth;
	uint8_t iRows = m_uiHeight;
	int border = 3;
	hexColor_t bg(background);
	// uint8_t* iBG = (uint8_t*)&background;
	float fScale = (float)m_uiBrightness/255.f;
	for (int i=1; i<4; i++)
		bg.bytes[i] = ((float)bg.bytes[i])*fScale;

	float fNone[4] = {0,0,0,1};
	if (bMaterial)
	{
		float fCopy[4] = { float(bg.red)/255.f,float(bg.green)/255.f,float(bg.blue)/255.f,0.0f};
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE | GL_SPECULAR, fNone);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION , fCopy);
	}
	else
		glColor4ub(bg.red,bg.green,bg.blue,bg.alpha);

	glTranslatef(border, border, 0);
	glBegin(GL_QUADS);
		glVertex3f(iCols * m_uiCharW + (iCols - 1) + border, -border, 0);
		glVertex3f(-border, -border, 0);
		glVertex3f(-border, iRows * m_uiCharH + (iRows - 1) + border, 0);
		glVertex3f(iCols * m_uiCharW + (iCols - 1) + border, iRows * m_uiCharH
				+ (iRows - 1) + border, 0);
	glEnd();
	for (int v = 0 ; v < m_uiHeight; v++) {
		glPushMatrix();
		for (int i = 0; i < m_uiWidth; i++) {
			GLPutChar(m_vRam[m_lineOffsets[v] + i], character, text, shadow, bMaterial);
			glTranslatef(6, 0, 0);
		}
		glPopMatrix();
		glTranslatef(0, 9, 0);
	}
	//SetFlag(HD44780_FLAG_DIRTY, 0);
}
