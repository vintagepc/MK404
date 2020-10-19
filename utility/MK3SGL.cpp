/*
	MK3SGL.cpp - Printer visualization for a MK3S, with MMU and print.

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

#include "MK3SGL.h"
#include "Camera.hpp"         // for Camera
#include "GLPrint.h"          // for GLPrint
#include "HD44780GL.h"        // for HD44780GL
#include "KeyController.h"
#include "MK3S_Bear.h"        // for MK3S_Bear
#include "MK3S_Full.h"        // for MK3S_Full
#include "MK3S_Lite.h"        // for MK3S_Lite
#include "Macros.h"
#include "OBJCollection.h"    // for OBJCollection, OBJCollection::ObjClass
#include "gsl-lite.hpp"
//NOLINTNEXTLINE _std must come before _ext.
#include <GL/freeglut_std.h>  // for glutSetWindow, GLUT_DOWN, GLUT_UP, glut...
#ifndef TEST_MODE
#include <GL/freeglut_ext.h>  // for glutSetOption
#endif
#include <GL/glew.h>          // for glTranslatef, glPopMatrix, glPushMatrix
#include <cstdlib>           // for exit
#include <cstring>
#include <iostream>             // for size_t, fprintf, printf, stderr
#include <vector>             // for vector

MK3SGL* MK3SGL::g_pMK3SGL = nullptr;

MK3SGL::MK3SGL(const std::string &strModel, bool bMMU, Printer *pParent):Scriptable("3DVisuals"), IKeyClient(),m_bMMU(bMMU),m_pParent(pParent)
{
	if (g_pMK3SGL)
	{
		std::cerr << "ERROR: Cannot have multiple MK3SGL instances due to freeglut limitations." << '\n';
		exit(1);
	}
	g_pMK3SGL = this;
	if (strModel == "lite")
	{
		m_Objs = new MK3S_Lite(bMMU);
	}
	else if (strModel == "fancy")
	{
		m_Objs = new MK3S_Full(bMMU);
	}
	else if (strModel == "bear")
	{
		m_Objs = new MK3S_Bear(bMMU);
	}

	RegisterActionAndMenu("ClearPrint","Clears rendered print objects",ActClear);
	RegisterActionAndMenu("ToggleNozzleCam","Toggles between normal and nozzle cam mode.",ActToggleNCam);
	RegisterActionAndMenu("ResetCamera","Resets camera view to default",ActResetView);
	RegisterAction("MouseBtn", "Simulates a mouse button (# = GL button enum, gl state,x,y)", ActMouse, {ArgType::Int,ArgType::Int,ArgType::Int,ArgType::Int});
	RegisterAction("MouseMove", "Simulates a mouse move (x,y)", ActMouseMove, {ArgType::Int,ArgType::Int});
	RegisterActionAndMenu("NonLinearX", "Toggle motor nonlinearity on X", ActNonLinearX);
	RegisterActionAndMenu("NonLinearY", "Toggle motor nonlinearity on Y", ActNonLinearY);
	RegisterActionAndMenu("NonLinearZ", "Toggle motor nonlinearity on Z", ActNonLinearZ);
	RegisterActionAndMenu("NonLinearE", "Toggle motor nonlinearity on E", ActNonLinearE);

	RegisterKeyHandler('`', "Reset camera view to default");
	RegisterKeyHandler('n',"Toggle Nozzle-Cam Mode");
	RegisterKeyHandler('l',"Clears any print on the bed. May cause graphical glitches if used while printing.");
	RegisterKeyHandler('w',"");
	RegisterKeyHandler('s',"");

	// RegisterKeyHandler('5',"");
	// RegisterKeyHandler('6',"");
	// RegisterKeyHandler('7',"");
	// RegisterKeyHandler('8',"");
	// RegisterKeyHandler('9',"");
	// RegisterKeyHandler('0',"");


	glewInit();
#ifdef TEST_MODE
	glutInitDisplayMode( US(GLUT_RGB) | US(GLUT_DOUBLE) | US(GLUT_DEPTH)) ;
#else
	glutSetOption(GLUT_MULTISAMPLE,4);
	glutInitDisplayMode( US(GLUT_RGB) | US(GLUT_DOUBLE) | US(GLUT_DEPTH) | US(GLUT_MULTISAMPLE)) ;
#endif
	glutInitWindowSize(800,800);		/* width=400pixels height=500pixels */
	std::string strTitle = std::string("Fancy Graphics: ") + m_Objs->GetName();
	m_iWindow = glutCreateWindow(strTitle.c_str());	/* create window */

	auto fcnDraw = []() { g_pMK3SGL->Draw();};
	glutDisplayFunc(fcnDraw);

	glutKeyboardFunc(KeyController::GLKeyReceiver); // same func as main window.

	auto fwd = [](int button, int state, int x, int y) {g_pMK3SGL->MouseCB(button,state,x,y);};
	glutMouseFunc(fwd);

	auto fcnMove = [](int x, int y) { g_pMK3SGL->MotionCB(x,y);};
	glutMotionFunc(fcnMove);

	auto fcnResize = [](int x, int y) { g_pMK3SGL->ResizeCB(x,y);};
	glutReshapeFunc(fcnResize);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
#ifndef TEST_MODE
	glEnable(GL_MULTISAMPLE);
#endif

	ResetCamera();

	m_Objs->Load();

	if (m_bMMU)
	{
		for(auto &o : m_vObjMMU)
		{
			o->Load();
		}
		m_MMUIdl.SetSubobjectVisible(1,false); // Screw, high triangle count
		m_MMUBase.SetSubobjectVisible(1, false);

		if (strModel == "lite")
		{
			m_MMUIdl.SetAllVisible(false);
			m_MMUIdl.SetSubobjectVisible(3);
			m_MMUSel.SetAllVisible(false);
			m_MMUSel.SetSubobjectVisible(1);
			m_MMUSel.SetSubobjectVisible(2);
			m_MMUBase.SetAllVisible(false);
			m_MMUBase.SetSubobjectVisible(17);
			for (size_t i=32; i<43; i++)
			{
				m_MMUBase.SetSubobjectVisible(i); // LEDs
			}
		}
	}

}

void MK3SGL::ResetCamera()
{
	m_camera = Camera();
	m_camera.setWindowSize(800,800);
	m_camera.setEye(0,0.5,3);
	m_camera.setCenter(0,0,0);
}

void MK3SGL::ResizeCB(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, static_cast<float>(w) / static_cast<float>(h), 0.01f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void MK3SGL::OnKeyPress(const Key& key)
{
	switch (key)
	{
		case 'l':
			ClearPrint();
			break;
		case 'n':
			ToggleNozzleCam();
		break;
		case '`':
			ResetCamera();
			break;
		case 'w':
		case 's':
			TwistKnob(key=='w');
			break;
		case '5':
		case '6':
			m_flDbg = m_flDbg + (key=='5'?0.001f:-0.001f);
			break;
		case '7':
		case '8':
			m_flDbg2 = m_flDbg2 + (key=='7'?0.001f:-0.001f);
			break;
		case '9':
		case '0':
			m_flDbg3 = m_flDbg3 + (key=='9'?0.001f:-0.001f);
			break;

	}
	// Decomment this block and use the flDbg variables
	// as your position translation. Then, you can move the
	// object into place using the numpad, and just read off
	// the correct position values to use when finished.
	// 	m_iDbg ++;
	// 	m_MMUBase.SetSubobjectVisible(m_iDbg,false);
	// }
	// else if (c == '8')
	// {
	// 	m_MMUBase.SetSubobjectVisible(m_iDbg,true);
	// 	m_iDbg --;
	// }
	// printf("Int: %d\n",m_iDbg.load());
	//printf("Offsets: %03f, %03f, %03f,\n",m_flDbg.load(),m_flDbg2.load(), m_flDbg3.load());
}

void MK3SGL::Init(avr_t *avr)
{
	_Init(avr,this);
	auto fcnCB = MAKE_C_CALLBACK(MK3SGL,OnPosChanged);
	RegisterNotify(X_IN,fcnCB,this);
	RegisterNotify(Y_IN,fcnCB,this);
	RegisterNotify(Z_IN,fcnCB,this);
	RegisterNotify(E_IN,fcnCB,this);
	RegisterNotify(FEED_IN,fcnCB,this);
	RegisterNotify(SEL_IN,fcnCB,this);
	RegisterNotify(IDL_IN,fcnCB,this);

	auto fcnMotor = MAKE_C_CALLBACK(MK3SGL, OnMotorStep);
	RegisterNotify(X_STEP_IN, fcnMotor, this);
	RegisterNotify(Y_STEP_IN, fcnMotor, this);
	RegisterNotify(Z_STEP_IN, fcnMotor, this);
	RegisterNotify(E_STEP_IN, fcnMotor, this);

	auto fcnBoolCB = MAKE_C_CALLBACK(MK3SGL,OnBoolChanged);
	RegisterNotify(SHEET_IN, 	fcnBoolCB, 	this);
	RegisterNotify(EFAN_IN, 	fcnBoolCB, 	this);
	RegisterNotify(PFAN_IN, 	fcnBoolCB, 	this);
	RegisterNotify(BED_IN, 		fcnBoolCB,	this);
	RegisterNotify(SD_IN, 		fcnBoolCB,	this);
	RegisterNotify(PINDA_IN, 	fcnBoolCB,	this);
	RegisterNotify(FINDA_IN, 	fcnBoolCB,	this);

	RegisterNotify(TOOL_IN,		MAKE_C_CALLBACK(MK3SGL,OnToolChanged),this);
	RegisterNotify(MMU_LEDS_IN,	MAKE_C_CALLBACK(MK3SGL,OnMMULedsChanged),this);

	m_bDirty = true;
}


Scriptable::LineStatus MK3SGL::ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs)
{
	if (m_iQueuedAct>=0) // Don't clobber a queued item...
	{
		return LineStatus::Waiting;
	}
	else if (m_iQueuedAct == -2)
	{
		m_iQueuedAct = -1;
		return LineStatus::Finished;
	}
	switch (iAct)
	{
		case ActMouse:
		case ActMouseMove:
			m_vArgs = vArgs;
		case ActResetView:
			m_iQueuedAct = iAct;
			return LineStatus::Waiting;
		case ActToggleNCam:
			m_bFollowNozzle = !m_bFollowNozzle;
			return LineStatus::Finished;
		case ActClear:
			ClearPrint();
			return LineStatus::Finished;
		case ActNonLinearX: // NOTE: we only do this for T0 for now. Not hard to extend with for(auto &print: m_vPrints)...
			std::cout << "Nonlinear X: " << std::to_string(m_Print.ToggleNLX()) << '\n';
			return LineStatus::Finished;
		case ActNonLinearY:
			std::cout << "Nonlinear Y: " << std::to_string(m_Print.ToggleNLY()) << '\n';
			return LineStatus::Finished;
		case ActNonLinearZ: // NOTE: we only do this for T0 for now. Not hard to extend with for(auto &print: m_vPrints)...
			std::cout << "Nonlinear Z: " << std::to_string(m_Print.ToggleNLZ()) << '\n';
			return LineStatus::Finished;
		case ActNonLinearE:
			std::cout << "Nonlinear E: " << std::to_string(m_Print.ToggleNLE()) << '\n';
			return LineStatus::Finished;
		default:
			return LineStatus::Unhandled;

	}
}

void MK3SGL::ProcessAction_GL()
{
	switch (m_iQueuedAct.load())
	{
		case ActResetView:
			ResetCamera();
		case ActMouse:
			MouseCB(std::stoi(m_vArgs.at(0)),std::stoi(m_vArgs.at(1)),std::stoi(m_vArgs.at(2)),std::stoi(m_vArgs.at(3)));
		case ActMouseMove:
			MotionCB(std::stoi(m_vArgs.at(0)),std::stoi(m_vArgs.at(1)));
	}
	m_iQueuedAct = -2;
}

void MK3SGL::TwistKnob(bool bDir)
{
	if (bDir)
	{
		m_iKnobPos = (m_iKnobPos+18)%360;
	}
	else
	{
		m_iKnobPos = (m_iKnobPos + 342)%360;
	}
}

void MK3SGL::OnBoolChanged(avr_irq_t *irq, uint32_t value)
{
	bool bVal = value>0;
	switch(irq->irq)
	{
		case IRQ::BED_IN:
			m_bBedOn = bVal;
			break;
		case IRQ::SHEET_IN:
			m_bPrintSurface = bVal;
			break;
		case IRQ::EFAN_IN:
			m_iFanPos = value;
			break;
		case IRQ::PFAN_IN:
			m_iPFanPos = value;
			break;
		case IRQ::SD_IN:
			m_bSDCard = !bVal; // CS is inverted.
			break;
		case IRQ::PINDA_IN:
			m_bPINDAOn = bVal;
			break;
		case IRQ::FINDA_IN:
			m_bFINDAOn = bVal;
			break;
		default:
		std::cout << "NOTE: MK3SGL: Unhandled Bool IRQ " << irq->name << '\n';
			break;
	}
	m_bDirty = true;
}

void MK3SGL::OnMMULedsChanged(avr_irq_t *irq, uint32_t value)
{
	// Set the materials appropriately:
	// Bits 9..0 are LEDs G/R 1..4, 0
	// LEDs are subobjects R: 32-36 (4..0) and G(38-42)
	//Red mtl = 31, G = 32. On mtls = 37/38
	// 	m_lGreen[0].ConnectFrom(m_shift.GetIRQ(HC595::BIT6), LED::LED_IN);
	// m_lRed[0].ConnectFrom(	m_shift.GetIRQ(HC595::BIT7), LED::LED_IN);
	// m_lGreen[4].ConnectFrom(m_shift.GetIRQ(HC595::BIT8), LED::LED_IN);
	// m_lRed[4].ConnectFrom(	m_shift.GetIRQ(HC595::BIT9), LED::LED_IN);
	// m_lGreen[3].ConnectFrom(m_shift.GetIRQ(HC595::BIT10), LED::LED_IN);
	// m_lRed[3].ConnectFrom(	m_shift.GetIRQ(HC595::BIT11), LED::LED_IN);
	// m_lGreen[2].ConnectFrom(m_shift.GetIRQ(HC595::BIT12), LED::LED_IN);
	// m_lRed[2].ConnectFrom(	m_shift.GetIRQ(HC595::BIT13), LED::LED_IN);
	// m_lGreen[1].ConnectFrom(m_shift.GetIRQ(HC595::BIT14), LED::LED_IN);
	// m_lRed[1].ConnectFrom(	m_shift.GetIRQ(HC595::BIT15), LED::LED_IN);
	uint32_t bChanged = irq->value ^ value;
	static constexpr uint8_t iLedBase[2] = {38, 32}; // G, R
	static constexpr uint8_t iLedObj[10] = {4,4,0,0,1,1,2,2,3,3};
	//static constexpr uint8_t iMtlOff[2] = {32, 31};
	static constexpr uint8_t iMtlOn[2] = {38,37};
	for (unsigned int i=0; i<10; i++)
	{
		if ((bChanged>>i) &1U)
		{
			if ((value>>i) & 1U)
			{
				m_MMUBase.SetSubobjectMaterial(gsl::at(iLedBase,i%2)+gsl::at(iLedObj,i),gsl::at(iMtlOn,i%2));
			}
			else
			{
				m_MMUBase.SetSubobjectMaterial(gsl::at(iLedBase,i%2)+gsl::at(iLedObj,i),5);//iMtlOff[i%2]);
			}
		}
	}

	m_bDirty = true;
}

void MK3SGL::OnMotorStep(avr_irq_t *irq, uint32_t value)
{
	switch (irq->irq)
	{
		case IRQ::X_STEP_IN:
			m_vPrints[m_iCurTool]->OnXStep(value);
			break;
		case IRQ::Y_STEP_IN:
			m_vPrints[m_iCurTool]->OnYStep(value);
			break;
		case IRQ::Z_STEP_IN:
			m_vPrints[m_iCurTool]->OnZStep(value);
			break;
		case IRQ::E_STEP_IN:
			// The narrow will assert if there is an overflow/underflow condition.
			// Time should never be negative...
			m_vPrints[m_iCurTool]->OnEStep(value, gsl::narrow<uint32_t>(m_pAVR->cycle - m_lastETick));
			m_lastETick = m_pAVR->cycle;
			break;
	}
}

void MK3SGL::OnPosChanged(avr_irq_t *irq, uint32_t value)
{
	float fPos;
	std::memcpy(&fPos,&value,4);
	switch (irq->irq)
	{
		case IRQ::X_IN:
			m_fXPos = fPos/1000.f;
			break;
		case IRQ::Y_IN:
			m_fYPos = fPos/1000.f;
			break;
		case IRQ::Z_IN:
			m_fZPos = fPos/1000.f;
			break;
		case IRQ::E_IN:
			m_fEPos = fPos/1000.f;
			break;
		case IRQ::FEED_IN:
			m_fPPos = fPos/1000.f;
			break;
		case IRQ::IDL_IN:
			m_fIdlPos = fPos;
			break;
		case IRQ::SEL_IN:
			m_fSelPos = fPos/1000.f;
			break;
		default:
			std::cout << "NOTE: MK3SGL: Unhandled IRQ " << irq->name << '\n';
			break;
	}
	m_bDirty = true;
}

void MK3SGL::OnToolChanged(avr_irq_t *, uint32_t iIdx)
{
	// Need to stop the old tool and start the new one at the right location.
	m_iCurTool = iIdx;
};

static constexpr float fMM2M = 1.f/1000.f;

void MK3SGL::Draw()
{
		//if (!m_bDirty)
		//    return;
	if (m_bClearPrints) // Needs to be done in the GL loop for thread safety.
	{
		for (int i=0; i<5; i++) m_vPrints[i]->Clear();
		m_bClearPrints = false;
	}
	if (m_iQueuedAct>=0)
	{
		// This may have issues if draw() is reentrant but I don't think it is as GLUT
		// waits for the draw call to finish before it kicks off another one from PostRedisplay
		ProcessAction_GL();
	}
	glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
	glClearDepth(1.0f);
	glClear( US(GL_COLOR_BUFFER_BIT) | US(GL_DEPTH_BUFFER_BIT));

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glLoadIdentity();

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		float fExtent = m_Objs->GetScaleFactor();
		m_Objs->SetupLighting();

		glEnable(GL_LIGHT0);

		glEnable(GL_LIGHTING);
		glMatrixMode(GL_MODELVIEW);
		glEnable(GL_NORMALIZE);
		glLoadIdentity();

		if (!m_bFollowNozzle)
		{
			float fSize = 0.1f;
			float fBase[3] = {0,0,0};
			glLineWidth(2.f);
			// If a mouse button is pressed, draw the "look at" axis.
			glPushMatrix();
				glEnable(GL_COLOR_MATERIAL);
				glBegin(GL_LINES);
					glColor3f(1,0,0);
					glVertex3f(fBase[0]-fSize, fBase[1], fBase[2]); glVertex3f(fBase[0]+fSize,fBase[1], fBase[2]);
					glColor3f(0,1,0);
					glVertex3f(fBase[0], fBase[1]-fSize,  fBase[2]); glVertex3f(fBase[0], fBase[1]+fSize, fBase[2]);
					glColor3f(0,0,1);
					glVertex3f(fBase[0],fBase[1], fBase[2]-fSize); glVertex3f(fBase[0],fBase[1], fBase[2]+fSize);
				glEnd();
				glDisable(GL_COLOR_MATERIAL);
			glPopMatrix();
			glMultMatrixf(m_camera.getViewMatrix());
		}

		glScalef(1.0f / fExtent, 1.0f / fExtent, 1.0f / fExtent);

		float _fTransform[3] = {0,0,0};
		gsl::span<float> fTransform {_fTransform};
		m_Objs->GetBaseCenter(fTransform);
		// Centerize object.
		glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
		if (m_bFollowNozzle)
		{
				float _fLook[3] = {0,0,0};
				gsl::span<float> fLook {_fLook};
				m_Objs->GetNozzleCamPos(fLook);
				fLook[0]+=fTransform[0]=m_fXPos;
				fLook[1]+=m_fZPos;
				gluLookAt(fLook[0]+.001, fLook[1]+.003 ,fLook[2]+.08, fLook[0],fLook[1],fLook[2] ,0,1,0);
		}
		m_Objs->SetNozzleCam(m_bFollowNozzle);
		glPushMatrix();
			glTranslatef(0,m_fZPos,0);
			m_Objs->Draw(OBJCollection::ObjClass::Z);
			glPushMatrix();
				glTranslatef(m_fXPos,0,0);
				m_Objs->Draw(OBJCollection::ObjClass::X);
				if (!m_bPINDAOn)
				{
					glPushMatrix();
						m_Objs->ApplyPLEDTransform();
						DrawRoundLED();
					glPopMatrix();
				}
				glPushMatrix();
					m_Objs->DrawPFan(m_iPFanPos);
				glPopMatrix();

				glPushMatrix();
					m_Objs->DrawEFan(m_iFanPos);
				glPopMatrix();
				glPushMatrix();
					m_Objs->DrawEVis(m_fEPos);
				glPopMatrix();
			glPopMatrix();
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0,0,(m_fYPos));
			m_Objs->Draw(OBJCollection::ObjClass::Y);
			if (m_bPrintSurface)
			{
				m_Objs->Draw(OBJCollection::ObjClass::PrintSurface);
			}
			glPushMatrix();
				glScalef(1,1,-1);
				m_Objs->ApplyPrintTransform();
				for (auto &c : m_vPrints)
				{
					c->Draw();
				}
			glPopMatrix();
			if (m_bBedOn)
			{
				glTranslatef(m_flDbg,m_flDbg2,m_flDbg3);
				m_Objs->ApplyBedLEDTransform();
				DrawLED(1,0,0);
			}
		glPopMatrix();
		m_Objs->Draw(OBJCollection::ObjClass::Fixed);
		glPushMatrix();
			m_Objs->DrawKnob(m_iKnobPos);
		glPopMatrix();
		if (m_pLCD)
		{
			glPushMatrix();
				m_Objs->ApplyLCDTransform();
				glRotatef(-45.f,1,0,0);
				glPushMatrix();
				float fScale = (4.f*0.076f)/500.f; // Disp is 76mm wide, lcd is drawn 500 wide at 4x scale
					glScalef(fScale,fScale,fScale);
					glScalef(1.0,-1.0f,-0.1f);
					glPushMatrix();
						// This scheme is a bit more legible on the smaller screen.
						m_pLCD->Draw(0x382200ff, 0x000000ff , 0xFF9900ff, 0x00000055, true);
					glPopMatrix();
				glPopMatrix();
			glPopMatrix();
		}
		if (m_bSDCard) //if card present
		{
			m_Objs->Draw(OBJCollection::ObjClass::Media); // Draw removable media (SD, USB, etc)
		}
		if (m_Objs->SupportsMMU() && m_bMMU)
		{
			DrawMMU();
		}
		m_snap.OnDraw();
		glutSwapBuffers();
		m_bDirty = false;
}

void MK3SGL::DrawRoundLED()
{
	std::vector<float> vLED = {1,0,0,1};
	std::vector<float> vNone = {0,0,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, US(GL_AMBIENT_AND_DIFFUSE) | US(GL_SPECULAR) ,vNone.data());
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,vLED.data());
	glPushMatrix();
		glTranslatef(0.092,0.3355,0.274);
		glBegin(GL_TRIANGLE_FAN);
			glVertex3f( 0,   0,    0);
			glVertex3f(-0.003,0,0);
			glVertex3f(-0.002,0,0.002);
			glVertex3f( 0,   0,     0.003);
			glVertex3f( 0.002,0,0.002);
			glVertex3f( 0.003,  0,  0);
			glVertex3f(0.002, 0,   -0.002);
			glVertex3f( 0,   0,    -0.003);
			glVertex3f(-0.002, 0, -0.002);
			glVertex3f(-0.003,0,0);
		glEnd();
	glPopMatrix();
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,vNone.data());
}

void MK3SGL::DrawLED(float r, float g, float b)
{
		std::vector<float> vLED = {r,g,b,1};
		std::vector<float> vNone = {0,0,0,1};
		glMaterialfv(GL_FRONT_AND_BACK, US(GL_AMBIENT_AND_DIFFUSE) | US(GL_SPECULAR),vNone.data());
				glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,vLED.data());
				glBegin(GL_QUADS);
					glVertex3f(0,0,0);
					glVertex3f(0.004,0,0);
					glVertex3f(0.004,0.003,0.003);
					glVertex3f(0,0.003,0.003);
				glEnd();
				glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION, vNone.data());
}

void MK3SGL::DrawMMU()
{
		float _fTransform[3] {0,0,0};
		gsl::span<float> fTransform = {_fTransform};
		glPushMatrix();
			m_MMUBase.GetCenteringTransform(fTransform);
			glTranslatef(0,0.3185,0.0425);
			glTranslatef(-fTransform[0], -fTransform[1], -fTransform[2]);
			glRotatef(-45,1,0,0);
			glTranslatef(0.13+fTransform[0],fTransform[1],fTransform[2]);
			m_MMUBase.Draw();
			glPushMatrix();
				m_MMUSel.GetCenteringTransform(fTransform);
				glPushMatrix();
					glTranslatef(m_fSelPos - m_fSelCorr,0.062,0.123);
					if (m_bFINDAOn)
					{
						glPushMatrix();
							glTranslatef(-0.075,-0.274,-0.222);
							DrawRoundLED();
						glPopMatrix();
					}
					glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
					glRotatef(-90,1,0,0);
					glRotatef(-2.5,0,1,0);
					glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
					m_MMUSel.Draw();
				glPopMatrix();
				glPushMatrix();
					glTranslatef(0.051, -0.299, -0.165);
					glScalef(fMM2M,fMM2M,fMM2M);
					m_EVis.GetCenteringTransform(fTransform);
					fTransform[1] +=1.5f;
					glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
					glRotatef(90,0,1,0);
					glRotatef(360.f*(m_fSelPos/0.008f),0,0,1); // 8mm thread pitch, so we need to rotate 360deg for every 8 of travel.
					glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
					m_EVis.Draw();
				glPopMatrix();
			glPopMatrix();

			glPushMatrix();
				m_MMUIdl.GetCenteringTransform(fTransform);
				glTranslatef(-0.03,0.028,0.025);
				fTransform[1]=-0.071;
				fTransform[2]=-0.0929;
				glPushMatrix();
					glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
					glRotatef(m_fIdlPos + m_fIdlCorr,1,0,0);
					glRotatef(180,0,1,0);
					glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
					m_MMUIdl.Draw();
				glPopMatrix();
				glPushMatrix();
					glTranslatef(-0.117, -0.300, -0.238);
					glScalef(fMM2M,fMM2M,fMM2M);
					m_EVis.GetCenteringTransform(fTransform);
					fTransform[1] +=1.5f;
					glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
					glRotatef(270,0,1,0);
					glRotatef(-m_fIdlPos,0,0,1);
					glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
					m_EVis.Draw();
				glPopMatrix();


			glPopMatrix();

			glPushMatrix();
				glTranslatef(0.061, -0.296, -0.212);
				glScalef(fMM2M,fMM2M,fMM2M);
				m_EVis.GetCenteringTransform(fTransform);
				fTransform[1] +=1.5f;
				glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
				glRotatef(90,0,1,0);
				glRotatef((m_fPPos/0.021f)*360.f,0,0,1); // 1 rotation for every 21mm of travel.
				glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
				m_EVis.Draw();
			glPopMatrix();

		glPopMatrix();
	}


void MK3SGL::MouseCB(int button, int action, int x, int y)
{
	auto w = glutGet(GLUT_WINDOW_WIDTH);
	auto h = glutGet(GLUT_WINDOW_HEIGHT);
	m_camera.setWindowSize(w, h);
	m_camera.setCurrentMousePos(x,y);
 	if (button == GLUT_LEFT_BUTTON) {
		if (action == GLUT_DOWN)
		{
			m_camera.beginRotate();
		}
		else if (action == GLUT_UP)
		{
			m_camera.endRotate();
		}
	}
	if (button == GLUT_RIGHT_BUTTON) {
		if (action == GLUT_DOWN)
		{
			m_camera.beginPan();
		}
		else if (action == GLUT_UP)
		{
			m_camera.endPan();
		}

	}
	if (button == GLUT_MIDDLE_BUTTON) {
		if (action == GLUT_DOWN)
		{
			m_camera.beginZoom();
		}
		else if (action == GLUT_UP)
		{
			m_camera.endZoom();
		}

	}
	if (button==3)
	{
		m_camera.zoom(0.5f);
	}
	if (button==4)
	{
		m_camera.zoom(-0.5f);
	}

	m_bDirty = true;
}

void MK3SGL::MotionCB(int x, int y)
{
 	m_camera.setCurrentMousePos(x, y);
	m_bDirty = true;
}
