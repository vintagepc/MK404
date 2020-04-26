/*
	thermistor.c

	Copyright 2008-2012 Michel Pollet <buserror@gmail.com>

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

#include "GL/glut.h"
#include "led.h"
#include "sim_avr.h"

static void value_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    led_t *this = (led_t*) param;
	this->bOn = value;
}

void drawLED_gl(led_t *this)
{
    glPushMatrix();
        if (this->bOn)
            glColor3f(this->fColor[0], this->fColor[1], this->fColor[2]);
        else
            glColor3f(this->fColor[0]/10, this->fColor[1]/10, this->fColor[2]/10);

        glBegin(GL_QUADS);
            glVertex2f(0,10);
            glVertex2f(10,10);
            glVertex2f(10,0);
            glVertex2f(0,0);
        glEnd();
        glColor3f(!this->bOn,!this->bOn,!this->bOn);
        glTranslatef(3,7,-1);
        glScalef(0.05,-0.05,1);
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,this->chrLabel);
    glPopMatrix();
}

static const char * irq_names[IRQ_LED_COUNT] = {
	[IRQ_LED_IN] = "<led.in"
};


void
led_init(
		avr_t *avr,
		led_t *this,
		uint32_t uiHexColor, char chrLabel)
{
    this->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_LED_COUNT, irq_names);
    avr_irq_register_notify(this->irq + IRQ_LED_IN,value_in_hook,this);
    this->bOn = false;
    this->chrLabel = chrLabel;
    //RGBA
    this->fColor[0] = (float)(uiHexColor >> 24)/255.0f;
    this->fColor[1] = (float)((uiHexColor >> 16)& 0xFF)/255.0f;
    this->fColor[2] = (float)((uiHexColor >> 8)& 0xFF)/255.0f;
	
}
