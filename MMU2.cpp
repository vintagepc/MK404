/*
	MMU2.cpp - A Missing-materials-unit for MK404

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

#include "MMU2.h"
#include <stdio.h>
#include "avr_uart.h"
#include "GL/glut.h"

// Yes yes, globals are bad. But we don't have any choice because freeglut calls 
// don't have parameter void pointers to retain the c++ class pointer. :-/
MMU2 *MMU2::g_pMMU = nullptr;
using namespace Boards;

MMU2::MMU2():MM_Control_01()
{
	if (g_pMMU)
	{
		fprintf(stderr,"Error: Cannot have multiple MMU instances due to freeglut limitations\n");
		exit(1);
	}
	g_pMMU = this;
	CreateBoard();
}

void* MMU2::Run()
{
	printf("Starting MMU2 execution...\n");
	int state = cpu_Running;
	while ((state != cpu_Done) && (state != cpu_Crashed) && !m_bQuit){	
		if (m_bReset)
		{
			m_bReset = 0;
			avr_reset(MM_Control_01::m_pAVR);
		}
		//if (gbPrintPC)
		//	printf("PC: %x\n",mmu->pc);
		state = avr_run(MM_Control_01::m_pAVR);
	}
	avr_terminate(MM_Control_01::m_pAVR);
	printf("MMU finished.\n");
	return NULL;
}

std::string MMU2::GetSerialPort()
{
	return m_UART.GetSlaveName();
}

void MMU2::Draw()		/* function called whenever redisplay needed */
{
	float fY = (float)(glutGet(GLUT_WINDOW_HEIGHT)/4);
	glPushMatrix();
		glColor3f(0,0,0);
		glTranslatef(0,fY-50,0);
		 glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
		glEnd();
		glTranslatef(20,7,0);
        glColor3f(1,1,1);
		glScalef(0.09,-0.05,0);
		for (int i=0; i<m_strTitle.size(); i++)
			glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,m_strTitle[i]);
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glTranslatef(0,fY-40,0);
		m_Sel.Draw();
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glTranslatef(0,fY-30,0);
		m_Idl.Draw();
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glTranslatef(0,fY-20,0);
		m_Extr.Draw_Simple();
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);		
		glTranslatef(0,fY-10,0);
		glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
		glEnd();
		for (int i=0; i<5; i++)
		{
			m_lRed[i].Draw();
			glTranslatef(20,0,0);
			m_lGreen[i].Draw();
			glTranslatef(40,0,0);
		}
		m_lFINDA.Draw();
	glPopMatrix();

}

void MMU2::SetupHardware()
{

	Boards::MM_Control_01::SetupHardware();

	_Init(GetAVR(), this);

	RegisterNotify(RESET,MAKE_C_CALLBACK(MMU2,OnResetIn),this);
	RegisterNotify(PULLEY_IN, MAKE_C_CALLBACK(MMU2,OnPulleyFeedIn),this);

	m_Sel.ConnectTo(TMC2130::POSITION_OUT,GetIRQ(SELECTOR_OUT));
	m_Idl.ConnectTo(TMC2130::POSITION_OUT,GetIRQ(IDLER_OUT));
	m_Extr.ConnectTo(TMC2130::POSITION_OUT,GetIRQ(PULLEY_IN));
	avr_irq_register_notify(m_shift.GetIRQ(HC595::OUT), MAKE_C_CALLBACK(MMU2,LEDHandler),this);
}


void MMU2::OnResetIn(struct avr_irq_t *irq, uint32_t value)
{
	//printf("MMU RESET: %02x\n",value);
	if (!value && !m_bStarted)
		Start();
    else if (irq->value && !value)
        m_bReset = true;
}

void MMU2::OnPulleyFeedIn(struct avr_irq_t * irq,uint32_t value)
{
	float* posOut = (float*)(&value);
   	SetPin(FINDA_PIN,posOut[0]>24.0f);

	// Reflect the distance out for IR sensor triggering.
	RaiseIRQ(FEED_DISTANCE, value);
}

void MMU2::LEDHandler(avr_irq_t *irq, uint32_t value)
{
	uint32_t valOut = 0;
	valOut = (value >>6) & 0b1111111111; // Just the LEDs.
	if (GetIRQ(LEDS_OUT)->value != valOut)
		RaiseIRQ(LEDS_OUT,valOut);
}

void MMU2::Start()
{
    if (m_bStarted)
        return; 
    printf("Starting MMU...\n");
    m_bStarted = true;

	auto fRunCB =[](void * param) { MMU2* p = (MMU2*)param; return p->Run();};

	pthread_create(&m_tRun, NULL, fRunCB, this);
}

void MMU2::Stop()
{
	printf("MMU_stop()\n");
    if (!m_bStarted)
        return;
    m_bQuit = true;
    pthread_join(m_tRun,NULL);
    printf("MMU Done\n");
}
