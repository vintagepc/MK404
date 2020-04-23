/*
	hd44780_glut.c

	Copyright Luki <humbell@ethz.ch>
	Copyright 2011 Michel Pollet <buserror@gmail.com>

 	This file is part of simavr.

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "hd44780_glut.h"

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

static int charwidth = 5;
static int charheight = 8;

void
hd44780_gl_init()
{
}

static inline void
glColor32U(uint32_t color)
{
	glColor4f(
			(float)((color >> 24) & 0xff) / 255.0f,
			(float)((color >> 16) & 0xff) / 255.0f,
			(float)((color >> 8) & 0xff) / 255.0f,
			(float)((color) & 0xff) / 255.0f );
}

void
glputchar(char c,
		uint32_t character,
		uint32_t text,
		uint32_t shadow, 
		uint8_t *pCGRAM)
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
	uint8_t rows=8;
	if (c<16)
		uiData = &pCGRAM[(c & 7) <<3];
	else
	{
		uiData = &hd44780_ROM_AOO.data[c*hd44780_ROM_AOO.h];
		rows = 7;
	}
		
	for (int i=0; i < rows; i++)
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

void
hd44780_gl_draw(
		hd44780_t *b,
		uint32_t background,
		uint32_t character,
		uint32_t text,
		uint32_t shadow)
{
	int rows = b->w;
	int lines = b->h;
	int border = 3;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	uint8_t* iBG = (uint8_t*)&background;
	float r = iBG[3]/255.0,g = iBG[2]/255.0,blue = iBG[1]/255.0;
	float fScale = (float)b->iBrightness/255.0;
	glColor4f(r*fScale,g*fScale,blue*fScale,1.0f);
	glTranslatef(border, border, 0);
	glBegin(GL_QUADS);
	glVertex3f(rows * charwidth + (rows - 1) + border, -border, 0);
	glVertex3f(-border, -border, 0);
	glVertex3f(-border, lines * charheight + (lines - 1) + border, 0);
	glVertex3f(rows * charwidth + (rows - 1) + border, lines * charheight
	        + (lines - 1) + border, 0);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	// TODO: Something is not right with these offsets. The stock ones
	// (0, 0x20,0x40,0x60) were totally whack-o, these seem to kinda-sorta work but
	// there are still lingering display quirks where stuff renders offscreen. Probably the actual memory
	// mapping needs to change instead of just the offsets since I suspect there are still
	// corner cases that write into incorrect areas.
	const uint8_t offset[] = { 0, 0x40, 0+rows, 0x40 + rows};
	for (int v = 0 ; v < b->h; v++) {
		glPushMatrix();
		for (int i = 0; i < b->w; i++) {
			glputchar(b->vram[offset[v] + i], character, text, shadow, b->cgram);
			glTranslatef(6, 0, 0);
		}
		glPopMatrix();
		glTranslatef(0, 9, 0);
	}
	hd44780_set_flag(b, HD44780_FLAG_DIRTY, 0);
}
