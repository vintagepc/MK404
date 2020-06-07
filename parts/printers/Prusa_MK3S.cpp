/*
	Prusa_MK3S.cpp - Printer definition for the Prusa MK3S
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

#include "Prusa_MK3S.h"
#include <GL/glut.h>
#include <stdio.h>

void Prusa_MK3S::Draw()
{
	glPushMatrix();
	glLoadIdentity(); // Start with an identity matrix
		glScalef(4, 4, 1);

		lcd.Draw(m_colors[(4*m_iScheme) + 0], /* background */
				m_colors[(4*m_iScheme) + 1], /* character background */
				m_colors[(4*m_iScheme) + 2], /* text */
				m_colors[(4*m_iScheme) + 3] /* shadow */ );
	glPopMatrix();

	// Do something for the motors...
	float fX = (5 + lcd.GetWidth()* 6)*4;
	float fY = (5 + lcd.GetHeight() * 9);
	glLoadIdentity();
	glScalef(fX/350,4,1);
	glPushMatrix();
		glTranslatef(0, fY,0);
		X.Draw();
		glTranslatef(0, 10,0);
		Y.Draw();
		glTranslatef(0, 10,0);
		Z.Draw();
		glTranslatef(0, 10,0);
		E.Draw_Simple();
		glTranslatef(250,0,0);
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

	if (GetVisualType()>VisualType::MINIMAL && m_pVis)
		m_pVis->Draw();

}

std::pair<int,int> Prusa_MK3S::GetWindowSize(){
	std::pair<int,int> prSize = { 5 + lcd.GetWidth() * 6,  40 + 5 + lcd.GetHeight() * 9};
	return prSize;
}

void Prusa_MK3S::OnVisualTypeSet(VisualType type)
{
	if (type==VisualType::MINIMAL)
		return;

	m_pVis = new MK3SGL(type==VisualType::MINIMAL,GetHasMMU());

	AddHardware(*m_pVis);

	m_pVis->ConnectFrom(X.GetIRQ(TMC2130::POSITION_OUT),MK3SGL::X_IN);
	m_pVis->ConnectFrom(Y.GetIRQ(TMC2130::POSITION_OUT),MK3SGL::Y_IN);
	m_pVis->ConnectFrom(Z.GetIRQ(TMC2130::POSITION_OUT),MK3SGL::Z_IN);
	m_pVis->ConnectFrom(E.GetIRQ(TMC2130::POSITION_OUT),MK3SGL::E_IN);
	m_pVis->ConnectFrom(pinda.GetIRQ(PINDA::SHEET_OUT), MK3SGL::SHEET_IN);
	m_pVis->ConnectFrom(fExtruder.GetIRQ(Fan::SPEED_OUT), MK3SGL::EFAN_IN);
	m_pVis->ConnectFrom(fPrint.GetIRQ(Fan::SPEED_OUT), MK3SGL::PFAN_IN);
	m_pVis->ConnectFrom(hBed.GetIRQ(Heater::ON_OUT), MK3SGL::BED_IN);
	m_pVis->ConnectFrom(sd_card.irq + IRQ_SD_CARD_PRESENT, MK3SGL::SD_IN);
	m_pVis->ConnectFrom(pinda.GetIRQ(PINDA::TRIGGER_OUT), MK3SGL::PINDA_IN);
	m_pVis->SetLCD(&lcd);
}

void Prusa_MK3S::FixSerial(avr_t * avr, avr_io_addr_t addr, uint8_t v)
{
	if (v==0x02)// Marlin is done setting up UCSRA0...
	{
		v|=(1<<5); // leave the UDRE0 alone
		printf("Reset UDRE0 after serial config changed\n");
	}
	avr_core_watch_write(avr,addr,v);
}

void Prusa_MK3S::SetupHardware()
{
	EinsyRambo::SetupHardware();

	if (GetConnectSerial())
		UART0.Connect('0');

	auto fcnSerial = [](avr_t *avr, avr_io_addr_t addr, uint8_t v, void * param)
		{Prusa_MK3S *p = (Prusa_MK3S*)param; p->FixSerial(avr, addr,v);};

	avr_register_io_write(m_pAVR, 0xC0, fcnSerial, this);
}

void Prusa_MK3S::OnAVRCycle()
{
	if (m_mouseBtn)
	{
		switch (m_mouseBtn){
			case 1:
				encoder.MousePush();
				break;
			case 2:
				encoder.Release();
				break;
			case 3:
				encoder.Twist(RotaryEncoder::CCW_CLICK);
				if (m_pVis) m_pVis->TwistKnob(true);
				break;
			case 4:
				encoder.Twist(RotaryEncoder::CW_CLICK);
				if (m_pVis) m_pVis->TwistKnob(false);
				break;
		}
		m_mouseBtn = 0;
	}
	if (m_key) {
		switch (m_key) {
			case 'w':
				printf("<");
				encoder.Twist(RotaryEncoder::CCW_CLICK);
				if (m_pVis) m_pVis->TwistKnob(true);
				break;
			case 's':
				printf(">");
				encoder.Twist(RotaryEncoder::CW_CLICK);
				if (m_pVis) m_pVis->TwistKnob(false);
				break;
			case 0xd:
				printf("ENTER pushed\n");
				encoder.Push();
				break;
			case 'r':
				printf("RESET/KILL\n");
				// RESET BUTTON
				SetResetFlag();
				encoder.Push(); // I dont' know why this is required to not get stuck in factory reset mode.
				// The only thing I can think of is that SimAVR doesn't like IRQ changes that don't have
				// any avr_run cycles between them. :-/
				break;
			case 't':
				printf("FACTORY_RESET\n");
				m_bFactoryReset =true;
				// Hold the button during boot to get factory reset menu
				SetResetFlag();
				break;
			case 'h':
				encoder.PushAndHold();
				break;
			case 'y':
				pinda.ToggleSheet();
				break;
			case 'p':
				printf("SIMULATING POWER PANIC\n");
				PowerPanic.Press(500);
				break;
			case 'f':
				IR.Toggle();
				break;
			case 'c':
				if (sd_card.data==NULL)
				{
					printf("Mounting SD image...\n");
					sd_card_mount_file(m_pAVR, &sd_card, sd_card.filepath,0);
				}
				else
				{
					printf("SD card removed...\n");
					sd_card_unmount_file(m_pAVR, &sd_card);
				}
				break;
			case 'q':
				Boards::EinsyRambo::SetQuitFlag();
				break;
		}
		m_key = 0;
	}
}

void Prusa_MK3S::OnMouseMove(int x,int y)
{
	// TODO - passthrough for vis.
}

void Prusa_MK3S::OnKeyPress(unsigned char key, int x, int y)
{
	switch (key) {
		case 'q':
			m_key = key;
			m_bPaused = false;
			break;
		case 'd':
			//gbPrintPC = gbPrintPC==0;
			break;
		case '1':
			m_iScheme ^=1;
			break;
		case 'z':
			m_bPaused ^= true;
			printf("Pause: %u\n",m_bPaused);
			break;
		case 'l':
			m_pVis->ClearPrint();
			break;
		case 'n':
			m_pVis->ToggleNozzleCam();
		break;
		case '`':
			m_pVis->ResetCamera();
			break;
		/* case 'r':
			printf("Starting VCD trace; press 's' to stop\n");
			avr_vcd_start(&vcd_file);
			break;
		case 's':
			printf("Stopping VCD trace\n");
			avr_vcd_stop(&vcd_file);
			break */;
		default:
			m_key = key;
	}
}

void Prusa_MK3S::OnMousePress(int button, int action, int x, int y)
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
