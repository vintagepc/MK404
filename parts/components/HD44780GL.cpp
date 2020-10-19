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

#include "HD44780GL.h"
#include "BasePeripheral.h"   // for MAKE_C_CALLBACK
#include "Macros.h"
#include "Util.h"             // for hexColor_t, hexColor_t::(anonymous)
#include "gsl-lite.hpp"
#include "hd44780_charROM.h"  // for (anonymous), hd44780_ROM_AOO
#include "sim_avr_types.h"    // for avr_regbit_t
#include "sim_regbit.h"       // for avr_regbit_get, AVR_IO_REGBIT

#include <GL/glew.h>
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glVertex3f, glBegin, glEnd, glMaterialfv
#else
# include <GL/gl.h>           // for glVertex3f, glBegin, glEnd, glMaterialfv
#endif
#include <mutex>
#include <vector>

//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif


static inline void
glColorHelper(const hexColor_t &color, bool bMaterial = false)
{

	if (bMaterial)
	{
		float fCol[4] = {
					static_cast<float>(color.red)/255.0f,
					static_cast<float>(color.green) / 255.0f,
					static_cast<float>(color.blue) / 255.0f,
					static_cast<float>(color.alpha) / 255.0f };
		float fNone[4] = {0,0,0,1};
		glMaterialfv(GL_FRONT_AND_BACK, US(GL_AMBIENT_AND_DIFFUSE) | US(GL_SPECULAR), static_cast<float*>(fNone));
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  static_cast<float*>(fCol));
	}
	else
	{
		glColor4ub(color.red, color.green, color.blue, color.alpha);
	}

}

HD44780GL::HD44780GL():HD44780(),IKeyClient()
{
	RegisterKeyHandler('1',"Toggles LCD Colour scheme");
}

void HD44780GL::OnKeyPress(const Key& key)
{
	if (key =='1')
	{
		m_iScheme = (m_iScheme+1)%2;
	}
}

void HD44780GL::Init(avr_t *avr)
{
	HD44780::Init(avr);

	RegisterNotify(BRIGHTNESS_IN, MAKE_C_CALLBACK(HD44780GL, OnBrightnessDigital),this);
	RegisterNotify(BRIGHTNESS_PWM_IN, MAKE_C_CALLBACK(HD44780GL, OnBrightnessPWM),this);
}

void HD44780GL::OnBrightnessPWM(struct avr_irq_t *, uint32_t value)
{
	//printf("Brightness pin changed value: %u\n",value);
	m_uiPWM = m_uiBrightness = value;
	SetFlag(HD44780_FLAG_DIRTY,1);
}

void HD44780GL::OnBrightnessDigital(struct avr_irq_t *,	uint32_t value)
{
	avr_regbit_t rb = AVR_IO_REGBIT(0x90,7); // COM3A1
	if (avr_regbit_get(m_pAVR,rb)) // Restore PWM value if being PWM-driven again after a digitalwrite
	{
		m_uiBrightness.store(m_uiPWM);
		return;
	}
	//printf("Brightness digital pin changed: %02x\n",value);
	if (value)
	{
		m_uiBrightness = 0xFF;
	}
	else
	{
		m_uiBrightness = 0x00;
	}
	SetFlag(HD44780_FLAG_DIRTY,1);

}

void HD44780GL::GenerateCharQuads()
{
	uint16_t uiCt = m_uiWidth*m_uiHeight;
	std::vector<float> vRects;
	vRects.reserve(uiCt*3*4); //4 points * 3 coords
	auto xShift = m_uiCharW+1;
	auto yShift = m_uiCharH+1;
	for (auto iRow = 0; iRow < m_uiHeight; iRow++)
	{
		for (auto iCol = 0; iCol<m_uiWidth; iCol++)
		{
			vRects.insert(vRects.end(),{5.f + (xShift*iCol), 8.f + (yShift*iRow), -1});
			vRects.insert(vRects.end(),{5.f + (xShift*iCol), 0.f + (yShift*iRow), -1});
			vRects.insert(vRects.end(),{0.f + (xShift*iCol), 0.f + (yShift*iRow), -1});
			vRects.insert(vRects.end(),{0.f + (xShift*iCol), 8.f + (yShift*iRow), -1});
		}
	}
	glGenBuffers(1, &m_bgVtxBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_bgVtxBuffer);
	glBufferData(GL_ARRAY_BUFFER, vRects.size() * sizeof(float), &vRects.at(0),	GL_STATIC_DRAW);
}

void HD44780GL::GLPutChar(unsigned char c, uint32_t character, uint32_t text, uint32_t shadow, bool bMaterial)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColorHelper(character,bMaterial);
	glBegin(GL_QUADS);
		glVertex3i(5, 8, -1);
		glVertex3i(5, 0, -1);
		glVertex3i(0, 0, -1);
		glVertex3i(0, 8, -1);
		auto uiData = hd44780_ROM_AOO.data.begin();
		uint8_t iCols=8;
		if (c<16)
		{
			uiData = m_cgRam.begin() + ((c & 7U) <<3U);
		}
		else
		{
			uiData += c*hd44780_ROM_AOO.h;
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
			for (uint8_t j=0; j<5; j++)
			{

				if (*uiData & (16U>>j))
				{
					auto x = static_cast<float>(j);
					auto y = static_cast<float>(i);
					float inset = 0.85;

					if (shadow)
					{
						glColorHelper(shadow, bMaterial);
						glVertex3f(x,y,		-2);
						glVertex3f(x,y+1,	-2);
						glVertex3f(x+1,y+1,	-2);
						glVertex3f(x+1,y,	-2);
					}
					glColorHelper(text, bMaterial);
					glVertex3f(x,y,				-3);
					glVertex3f(x,y+inset,		-3);
					glVertex3f(x+inset,y+inset,	-3);
					glVertex3f(x+inset,y,		-3);
				}
			}
			uiData++;
		}
	glEnd();
}

void HD44780GL::Draw(bool bMaterial)
{
	if (m_bgVtxBuffer==0)
	{
		GenerateCharQuads();
	}
	uint8_t iScheme = m_iScheme;
	Draw(m_colors.at((4*iScheme)), m_colors.at((4*iScheme)+1), m_colors.at((4*iScheme)+2), m_colors.at((4*iScheme)+3), bMaterial);
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
	float fScale = static_cast<float>(m_uiBrightness)/255.f;
	hexColor_t bg(background,fScale);

	glColorHelper(bg, bMaterial);

	glTranslatef(border, border, 0);
	glBegin(GL_QUADS);
		glVertex3f(iCols * m_uiCharW + (iCols - 1) + border, -border, 0);
		glVertex3f(-border, -border, 0);
		glVertex3f(-border, iRows * m_uiCharH + (iRows - 1) + border, 0);
		glVertex3f(iCols * m_uiCharW + (iCols - 1) + border, iRows * m_uiCharH
				+ (iRows - 1) + border, 0);
	glEnd();
	// glColorHelper(character,bMaterial);
	// glEnableClientState(GL_VERTEX_ARRAY);
	// 	glBindBuffer(GL_ARRAY_BUFFER, m_bgVtxBuffer);
	// 	glVertexPointer(3, GL_FLOAT, 3*sizeof(float), nullptr);
	// 	glDrawArrays(GL_QUADS, 0, 4*m_uiWidth*m_uiHeight);
	// glDisableClientState(GL_VERTEX_ARRAY);
	for (int v = 0 ; v < m_uiHeight; v++) {
		glPushMatrix();
		for (int i = 0; i < m_uiWidth; i++) {
			std::lock_guard<std::mutex> lock(m_lock);
			GLPutChar(m_vRam[m_lineOffsets.at(v) + i], character, text, shadow, bMaterial);
			glTranslatef(6, 0, 0);
		}
		glPopMatrix();
		glTranslatef(0, 9, 0);
	}
	//SetFlag(HD44780_FLAG_DIRTY, 0);
}
