/*
	Prusa_MK2_13.cpp - Printer definition for the Prusa MK2 (mR1.3)
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

#include "Prusa_MK2_13.h"
#include "A4982.h"            // for A4982, A4982::IRQ::POSITION_OUT, PINDA:...
#include "Beeper.h"           // for Beeper
#include "Fan.h"              // for Fan
#include "GLHelper.h"
#include "HD44780GL.h"        // for HD44780GL
#include "Heater.h"           // for Heater
#include "LED.h"              // for LED
#include "MMU1.h"             // for MMU1
#include "PINDA.h"            // for PINDA
#include "PinNames.h"         // for Pin::Z_MIN_PIN
#include "RotaryEncoder.h"    // for RotaryEncoder, RotaryEncoder::::CCW_CLICK
#include "sim_io.h"           // for avr_register_io_write
#include "uart_pty.h"         // for uart_pty
#include <GL/glew.h>          // NOLINT for glTranslatef, glLoadIdentity, glPopMatrix
#include <GL/freeglut_std.h>  // for GLUT_DOWN, GLUT_LEFT_BUTTON, GLUT_RIGHT...
#include <iostream>           // for operator<<, cout, ostream, basic_ostream

using std::cout;

Prusa_MK2_13::Prusa_MK2_13():MiniRambo(),Printer()
{
}

void Prusa_MK2_13::Draw()
{
		glPushMatrix();
		glLoadIdentity(); // Start with an identity matrix
			glScalef(4, 4, 1);
			lcd.Draw();
		glPopMatrix();
		// Do something for the motors...
		float fX = (5 + lcd.GetWidth()* 6)*4;
		float fY = (5 + lcd.GetHeight() * 9);
		glLoadIdentity();
		glScalef(fX/350,4,1);
		glPushMatrix();
			glTranslatef(0, fY,0);
			X.Draw();
			glTranslatef(0,10,0);
			Y.Draw();
			glTranslatef(0,10,0);
			Z.Draw();
			glTranslatef(0,10,0);
			glPushMatrix();
				E.Draw_Simple();
				glTranslatef(150.f,0,0);
				E1.Draw_Simple();
			glPopMatrix();
			glTranslatef(0,10,0);
			glPushMatrix();
				E2.Draw_Simple();
				glTranslatef(150.f,0,0);
				E3.Draw_Simple();
			glPopMatrix();
			glTranslatef(0,10,0);
			glTranslatef(170,0,0);
			m_mmu.Draw();
			glTranslatef(20,0,0);
			m_buzzer.Draw();
			glTranslatef(20,0,0);
			fPrint.Draw();
			glTranslatef(20,0,0);
			fExtruder.Draw();
			glTranslatef(20,0,0);
			hExtruder.Draw();
			glTranslatef(20,0,0);
			hBed.Draw();
			glTranslatef(20,0,0);
			lSD.Draw();
			glTranslatef(20,0,0);
			lPINDA.Draw();
			glTranslatef(20,0,0);
			lIR.Draw();
		glPopMatrix();
		// GL snapshot helper
		m_gl.OnDraw();
}

std::pair<int,int> Prusa_MK2_13::GetWindowSize(){
	std::pair<int,int> prSize = { 5 + lcd.GetWidth() * 6, 60 + 5 + lcd.GetHeight() * 9};
	return prSize;
}

void Prusa_MK2_13::FixSerial(avr_t * avr, avr_io_addr_t addr, uint8_t v)
{
	if (v==0x02)// Marlin is done setting up UCSRA0...
	{
		v|=(1<<5); // leave the UDRE0 alone
		cout << "Reset UDRE0 after serial config changed\n";
	}
	avr_core_watch_write(avr,addr,v);
}

void Prusa_MK2_13:: SetupPINDA()
{
	AddHardware(pinda, X.GetIRQ(A4982::POSITION_OUT),  Y.GetIRQ(A4982::POSITION_OUT),  Z.GetIRQ(A4982::POSITION_OUT));
	TryConnect(pinda, PINDA::TRIGGER_OUT ,Z_MIN_PIN);
	AddHardware(lPINDA);
	lPINDA.ConnectFrom(pinda.GetIRQ(PINDA::TRIGGER_OUT), LED::LED_IN);
}

void Prusa_MK2_13::SetupHardware()
{
	MiniRambo::SetupHardware();

	SetupPINDA();

	if (GetConnectSerial())
	{
		UART0.Connect('0');
	}

	auto fcnSerial = [](avr_t *avr, avr_io_addr_t addr, uint8_t v, void * param)
	{auto *p = static_cast<Prusa_MK2_13*>(param); p->FixSerial(avr, addr,v);};

	avr_register_io_write(m_pAVR, 0xC0, fcnSerial, this);

}

void Prusa_MK2_13::OnAVRCycle()
{
	int mouseBtn = m_mouseBtn;                  // copy atomic to local
	if (mouseBtn)
	{
		switch (mouseBtn){
			case 1:
				encoder.MousePush();
				break;
			case 2:
				encoder.Release();
				break;
			case 3:
				encoder.Twist(RotaryEncoder::CCW_CLICK);
				//if (m_pVis) m_pVis->TwistKnob(true);
				break;
			case 4:
				encoder.Twist(RotaryEncoder::CW_CLICK);
			//	if (m_pVis) m_pVis->TwistKnob(false);
				break;
		}
		m_mouseBtn = 0;
	}
	int key = m_key;                            // copy atomic to local
	if (key)
	{
		switch (key) {
			case 't':
				cout << "FACTORY_RESET\n";
				//m_bFactoryReset =true;
				// Hold the button during boot to get factory reset menu
				SetResetFlag();
				break;

		}
		m_key = 0;
	}
}

void Prusa_MK2_13::OnMouseMove(int /*x*/,int /*y*/)
{
}

void Prusa_MK2_13::OnMousePress(int button, int action, int /*x*/, int /*y*/)
{
	if (button == GLUT_LEFT_BUTTON || button == GLUT_RIGHT_BUTTON) {
		if (action == GLUT_DOWN) {
			m_mouseBtn = 1;
		} else if (action == GLUT_UP) {
			m_mouseBtn = 2;
		}
	}
	if ((button==3 || button==4) && action == GLUT_DOWN) // wheel
	{
		m_mouseBtn = button;
	}
}
