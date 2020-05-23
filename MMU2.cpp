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

#include "Util.h"
#include "MMU2.h"
#include <stdio.h>
#include "sim_hex.h"
#include <stdlib.h>
#include <string.h>
#include <avr_extint.h>
#include <avr_ioport.h>
#include "avr_uart.h"
#include "GL/glut.h"

// Yes yes, globals are bad. But we don't have any choice because freeglut calls 
// don't have parameter void pointers to retain the c++ class pointer. :-/
MMU2 *MMU2::g_pMMU = nullptr;

MMU2::MMU2()
{

}

void* MMU2::Run()
{
	printf("Starting MMU2 execution...\n");
	int state = cpu_Running;
	while ((state != cpu_Done) && (state != cpu_Crashed) && !m_bQuit){	
		if (m_bReset)
		{
			m_bReset = 0;
			avr_reset(m_pAVR);
		}
		//if (gbPrintPC)
		//	printf("PC: %x\n",mmu->pc);
		state = avr_run(m_pAVR);
	}
	avr_terminate(m_pAVR);
	printf("MMU finished.\n");
	return NULL;
}

void MMU2::Init()
{
		if (g_pMMU)
	{
		fprintf(stderr,"Error: Cannot have multiple MMU instances due to freeglut limitations\n");
		exit(1);
	}

	g_pMMU = this;
	uint32_t boot_base, boot_size;
	char mmcu[] = "atmega32u4";
	uint32_t freq = 16000000;
	avr_t *avr = avr_make_mcu_by_name(mmcu);

	if (!avr) {
		fprintf(stderr, "Error creating the MMU2 core\n");
		return;
	}
    char boot_path[] = "MM-control-01.hex";
	 uint8_t * boot = read_ihex_file(boot_path, &boot_size, &boot_base);
	 if (!boot) {
		fprintf(stderr, "MMU2: Unable to load %s\n", boot_path);
		return;
	 }
	printf("%s f/w 0x%05x: %d bytes\n", mmcu, boot_base, boot_size);
	avr_init(avr);

	_Init(avr, this);

	avr->frequency = freq;
	avr->vcc = 5000;
	avr->aref = 0;
	avr->avcc = 5000;
	memcpy(avr->flash + boot_base, boot, boot_size);
	printf("fw base at:%u\n",boot_base);
	free(boot);
    /* end of flash, remember we are writing /code/ */
	avr->codeend = avr->flashend;
	avr->log = 1;

	// even if not setup at startup, activate gdb if crashing
	avr->gdb_port = 1234;
	
	// suppress continuous polling for low INT lines... major performance drain.
	for (int i=0; i<5; i++)
		avr_extint_set_strict_lvl_trig(avr,i,false);

    m_UART0.Init(avr);

    printf("MMU UART:\n");
    m_UART0.Connect('1');

	SetupHardware();

	RegisterNotify(RESET,MAKE_C_CALLBACK(MMU2,OnResetIn),this);

	RegisterNotify(PULLEY_IN, MAKE_C_CALLBACK(MMU2,OnPulleyFeedIn),this);

	m_Extr.ConnectTo(TMC2130::POSITION_OUT,GetIRQ(PULLEY_IN));

	TMC2130::TMC2130_cfg_t cfg;
	cfg.uiStepsPerMM = 25;
	cfg.fStartPos = 0;
	cfg.cAxis = 'P';
	cfg.bHasNoEndStops = true;
	cfg.uiDiagPin = 30; // filler, not used.
	m_Extr.SetConfig(cfg);

	cfg.uiStepsPerMM = 8;
	cfg.iMaxMM = 200;
	cfg.cAxis = 'I';
	cfg.bHasNoEndStops = false;
	m_Idl.SetConfig(cfg);

	cfg.uiStepsPerMM = 50;
	cfg.cAxis = 'S';
	cfg.iMaxMM = 70;
	cfg.bInverted = true;
	m_Sel.SetConfig(cfg);
}

std::string MMU2::GetSerialPort()
{
	return m_UART0.GetSlaveName();
}

void MMU2::Draw()		/* function called whenever redisplay needed */
{
	glutSetWindow(m_iWindow);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW); // Select modelview matrix
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef((m_iWinW)*1.05/350,4,1);
		m_Sel.Draw();
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(m_iWinW*1.05/350,4,1);
		glTranslatef(0,10,0);
		m_Idl.Draw();
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(m_iWinW*1.05/350,4,1);
		glTranslatef(0,20,0);
		m_Extr.Draw_Simple();
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(m_iWinW*2.8/350,4,1);
		glTranslatef(0,30,0);
		glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
		glEnd();
		for (int i=0; i<5; i++)
		{
			m_lRed[i].Draw();
			glTranslatef(11,0,0);
			m_lGreen[i].Draw();
			glTranslatef(12,0,0);
		}
		m_lFINDA.Draw();
	glPopMatrix();

	glutSwapBuffers();

}

void MMU2::OnDisplayTimer(int i)
{
	auto fcnTimer = [](int i){g_pMMU->OnDisplayTimer(i);};
	glutTimerFunc(50, fcnTimer, 0);
	Draw();
}

void MMU2::InitGL()
{
	// Set up projection matrix
	glMatrixMode(GL_PROJECTION); // Select projection matrix
	glLoadIdentity(); // Start with an identity matrix
	glOrtho(0, m_iWinW, 0, m_iWinH, 0, 10);
	glScalef(1,-1,1);
	glTranslatef(0, -1 * m_iWinH, 0);

	auto fcnDraw = [](){g_pMMU->Draw();};

 	auto fcnTimer = [](int i){g_pMMU->OnDisplayTimer(i);};

	glutDisplayFunc(fcnDraw);		/* set window's display callback */
	glutTimerFunc(100, fcnTimer, 0);

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
}

void MMU2::SetupHardware()
{
	//There's only one shift register here that's 16 bits wide.
	// There are two on the MMU but they're chained, to the same effect.
	m_shift.Init(m_pAVR);

	avr_ioport_external_t ex = {.name = 'F', .mask=0b10011, .value=0 };
	avr_ioctl(m_pAVR, AVR_IOCTL_IOPORT_SET_EXTERNAL(ex.name),&ex);

	m_shift.ConnectFrom(IOIRQ(m_pAVR, 'B',6), 	HC595::IN_LATCH);
	m_shift.ConnectFrom(IOIRQ(m_pAVR, 'B',5), 	HC595::IN_DATA);
	m_shift.ConnectFrom(IOIRQ(m_pAVR, 'C',7), 	HC595::IN_CLOCK);
	avr_irq_register_notify(m_shift.GetIRQ(HC595::OUT), MAKE_C_CALLBACK(MMU2,LEDHandler),this);

	m_Extr.Init(m_pAVR);
	m_Extr.ConnectFrom(IOIRQ(m_pAVR,'C',6), TMC2130::SPI_CSEL);
	m_Extr.ConnectFrom(IOIRQ(m_pAVR,'B',4), TMC2130::STEP_IN);

	m_Sel.Init(m_pAVR);
	m_Sel.ConnectFrom(IOIRQ(m_pAVR,'D',7),	TMC2130::SPI_CSEL);
	m_Sel.ConnectFrom(IOIRQ(m_pAVR,'D',4),	TMC2130::STEP_IN);
	m_Sel.ConnectTo(TMC2130::POSITION_OUT,GetIRQ(SELECTOR_OUT));

	m_Idl.Init(m_pAVR);
	m_Idl.ConnectFrom(IOIRQ(m_pAVR,'B',7), TMC2130::SPI_CSEL);
	m_Idl.ConnectFrom(IOIRQ(m_pAVR,'D',6), TMC2130::STEP_IN);
	m_Idl.ConnectTo(TMC2130::POSITION_OUT,GetIRQ(IDLER_OUT));

	for (int i=0; i<5; i++)
	{
		m_lGreen[i] = LED(0x00FF00FF);
		m_lGreen[i].Init(m_pAVR);
		m_lRed[i] = LED(0xFF0000FF);
		m_lRed[i].Init(m_pAVR);
	}
	m_lFINDA = LED(0xFFCC00FF,'F');
	m_lFINDA.Init(m_pAVR);
	m_lFINDA.ConnectFrom(IOIRQ(m_pAVR, 'F',6), LED::LED_IN);

	avr_raise_irq(IOIRQ(m_pAVR,'F',6),0);

	m_Extr.ConnectFrom(m_shift.GetIRQ(HC595::BIT0),	TMC2130::DIR_IN);
	m_Extr.ConnectFrom(m_shift.GetIRQ(HC595::BIT1),	TMC2130::ENABLE_IN);
	m_Sel.ConnectFrom(m_shift.GetIRQ(HC595::BIT2), TMC2130::DIR_IN);
	m_Sel.ConnectFrom(m_shift.GetIRQ(HC595::BIT3), TMC2130::ENABLE_IN);
	m_Idl.ConnectFrom(m_shift.GetIRQ(HC595::BIT4), TMC2130::DIR_IN);
	m_Idl.ConnectFrom(m_shift.GetIRQ(HC595::BIT5), TMC2130::ENABLE_IN);
	m_lGreen[0].ConnectFrom(m_shift.GetIRQ(HC595::BIT6), LED::LED_IN);
	m_lRed[0].ConnectFrom(	m_shift.GetIRQ(HC595::BIT7), LED::LED_IN);
	m_lGreen[4].ConnectFrom(m_shift.GetIRQ(HC595::BIT8), LED::LED_IN);
	m_lRed[4].ConnectFrom(	m_shift.GetIRQ(HC595::BIT9), LED::LED_IN);
	m_lGreen[3].ConnectFrom(m_shift.GetIRQ(HC595::BIT10), LED::LED_IN);
	m_lRed[3].ConnectFrom(	m_shift.GetIRQ(HC595::BIT11), LED::LED_IN);
	m_lGreen[2].ConnectFrom(m_shift.GetIRQ(HC595::BIT12), LED::LED_IN);
	m_lRed[2].ConnectFrom(	m_shift.GetIRQ(HC595::BIT13), LED::LED_IN);
	m_lGreen[1].ConnectFrom(m_shift.GetIRQ(HC595::BIT14), LED::LED_IN);
	m_lRed[1].ConnectFrom(	m_shift.GetIRQ(HC595::BIT15), LED::LED_IN);

	m_buttons.Init(m_pAVR,5);

}

void MMU2::LEDHandler(avr_irq_t *irq, uint32_t value)
{
	uint32_t valOut = 0;
	valOut = (value >>6) & 0b1111111111; // Just the LEDs.
	if (GetIRQ(LEDS_OUT)->value != valOut)
		RaiseIRQ(LEDS_OUT,valOut);
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
   	avr_raise_irq(IOIRQ(m_pAVR,'F',6),posOut[0]>24.0f);

	// Reflect the distance out for IR sensor triggering.
	RaiseIRQ(FEED_DISTANCE, value);
}

void MMU2::StartGL()
{
	int pixsize = 4;
	m_iWinW = 20*6 * pixsize;
	m_iWinH = 40*pixsize;
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(m_iWinW,m_iWinH);		/* m_iWinW=400pixels m_iWinH=500pixels */
	m_iWindow = glutCreateWindow("Missing Material Unit 2");	/* create window */
	InitGL();
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

