#include "MK3SGL.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <cstring>

#include <trackball.h>

#include <GL/glew.h>
#include <GL/glut.h>
MK3SGL::MK3SGL(bool bLite, bool bMMU):m_bLite(bLite),m_bMMU(bMMU)
{
	trackball(curr_quat,0,0,0,0);
		eye[0] = 0.0f;
		eye[1] = 0.0f;
		eye[2] = 3.0f;

		lookat[0] = 0.0f;
		lookat[1] = 0.0f;
		lookat[2] = 0.0f;

		up[0] = 0.0f;
		up[1] = 1.0f;
		up[2] = 0.0f;

		for(int i=0; i<m_vObjLite.size(); i++)
			m_vObjLite[i]->Load();

		if (m_bLite)
		{
				m_Y.SetAllVisible(false);
				m_Y.SetSubobjectVisible(2); // heatbed, sheet
				m_Extruder.SetAllVisible(false);
				m_Extruder.SetSubobjectVisible(19); // V6
				m_Extruder.SetSubobjectVisible(20);
				m_Extruder.SetSubobjectVisible(1); // PINDA
				m_Extruder.SetSubobjectVisible(2);
		}
		else
			for(int i=0; i<m_vObj.size(); i++)
				m_vObj[i]->Load();
		if (m_bMMU)
		{
			for(int i=0; i<m_vObjMMU.size(); i++)
				m_vObjMMU[i]->Load();
			m_MMUIdl.SetSubobjectVisible(1,false); // Screw, high triangle count
		}

}

void MK3SGL::Init(avr_t *avr)
{
	_Init(avr,this);
	RegisterNotify(X_IN,MAKE_C_CALLBACK(MK3SGL,OnXChanged),this);
	RegisterNotify(Y_IN,MAKE_C_CALLBACK(MK3SGL,OnYChanged),this);
	RegisterNotify(Z_IN,MAKE_C_CALLBACK(MK3SGL,OnZChanged),this);
	RegisterNotify(E_IN,MAKE_C_CALLBACK(MK3SGL,OnEChanged),this);
	RegisterNotify(SEL_IN,MAKE_C_CALLBACK(MK3SGL,OnSelChanged),this);
	RegisterNotify(IDL_IN,MAKE_C_CALLBACK(MK3SGL,OnIdlChanged),this);
	RegisterNotify(SHEET_IN, MAKE_C_CALLBACK(MK3SGL, OnSheetChanged), this);
	RegisterNotify(EFAN_IN, MAKE_C_CALLBACK(MK3SGL, OnEFanChanged), this);
	RegisterNotify(PFAN_IN, MAKE_C_CALLBACK(MK3SGL, OnPFanChanged), this);
	RegisterNotify(BED_IN, MAKE_C_CALLBACK(MK3SGL, OnBedChanged), this);
	RegisterNotify(SD_IN, MAKE_C_CALLBACK(MK3SGL,OnSDChanged),this);
	RegisterNotify(PINDA_IN, MAKE_C_CALLBACK(MK3SGL,OnPINDAChanged),this);

	RegisterNotify(MMU_LEDS_IN,MAKE_C_CALLBACK(MK3SGL,OnMMULedsChanged),this);

	m_bDirty = true;
}

void MK3SGL::TwistKnob(bool bDir)
{
	if (bDir)
		m_iKnobPos = (m_iKnobPos+18)%360;
	else
		m_iKnobPos = (m_iKnobPos + 342)%360;
}

void MK3SGL::OnXChanged(avr_irq_t *irq, uint32_t value)
{
	float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
	m_fXPos =  fPos[0]/1000.f;
	m_bDirty = true;
}

void MK3SGL::OnSelChanged(avr_irq_t *irq, uint32_t value)
{
	float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
	m_fSelPos =  fPos[0]/1000.f;
	m_bDirty = true;
}

void MK3SGL::OnIdlChanged(avr_irq_t *irq, uint32_t value)
{
	float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
	m_fIdlPos =  fPos[0];
	m_bDirty = true;
}

void MK3SGL::OnBedChanged(avr_irq_t *irq, uint32_t value)
{
	m_bBedOn = value>0;
	m_bDirty = true;
}

void MK3SGL::OnSheetChanged(avr_irq_t *irq, uint32_t value)
{
	m_Sheet.SetAllVisible(value>0);
	m_bDirty = true;
}

void MK3SGL::OnEFanChanged(avr_irq_t *irq, uint32_t value)
{
	m_bFanOn = (value>0);
	m_bDirty = true;
}

void MK3SGL::OnPFanChanged(avr_irq_t *irq, uint32_t value)
{
	m_bPFanOn = (value>0);
	m_bDirty = true;
}

void MK3SGL::OnSDChanged(avr_irq_t *irq, uint32_t value)
{
	m_SDCard.SetAllVisible(value^1);
	m_bDirty = true;
}

void MK3SGL::OnPINDAChanged(avr_irq_t *irq, uint32_t value)
{
	m_bPINDAOn = value;
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
	static constexpr uint8_t iMtlOff[2] = {32, 31};
	static constexpr uint8_t iMtlOn[2] = {38,37}; 
	for (int i=0; i<10; i++)
	{
		if ((bChanged>>i) &1)
		{
			if ((value>>i) & 1)
				m_MMUBase.SetSubobjectMaterial(iLedBase[i%2]+iLedObj[i],iMtlOn[i%2]);
			else
				m_MMUBase.SetSubobjectMaterial(iLedBase[i%2]+iLedObj[i],5);//iMtlOff[i%2]);
		}
	}

	m_bDirty = true;
}

void MK3SGL::OnYChanged(avr_irq_t *irq, uint32_t value)
{
	float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
	m_fYPos =  fPos[0]/1000.f;
	m_bDirty = true;
}

void MK3SGL::OnEChanged(avr_irq_t *irq, uint32_t value)
{
	float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
	m_fEPos =  fPos[0]/1000.f;
	m_bDirty = true;
}

void MK3SGL::OnZChanged(avr_irq_t *irq, uint32_t value)
{
	float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
	m_fZPos =  fPos[0]/1000.f;
	m_bDirty = true;
}

void MK3SGL::Draw()
{
		//if (!m_bDirty)
		//    return;
		glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
	glLoadIdentity();
	//printf("eye: %f %f %f\n",curr_quat[0],curr_quat[1], curr_quat[2]);
		float pos[] = {2,2,2,0};
		float fpos[] = {0,1,-1,1};
		float fNone[] = {0,0,0,1};
		float fAmb[] = {.1,.1,.1,1};
		float fCol2[] = {1,1,1,1};
		float fSpec[] = {.4,.4,.4,.5};
		float fDiff[] = {1.5,1.5,1.5,1};
		float fLED[4] = {1,0,0,1};
		// camera & rotate

		glEnable(GL_CULL_FACE);

		glLightfv(GL_LIGHT0,GL_AMBIENT, fAmb);
		glLightfv(GL_LIGHT0,GL_SPECULAR, fSpec);
		glLightfv(GL_LIGHT0,GL_DIFFUSE, fDiff);
		glLightfv(GL_LIGHT0,GL_POSITION, pos);
		glEnable(GL_LIGHT0);

		glEnable(GL_LIGHTING); 
		glMatrixMode(GL_MODELVIEW);
		glEnable(GL_NORMALIZE);
		glLoadIdentity();
		GLfloat mat[4][4];
		if (!m_bFollowNozzle)
		{
			gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0],
							up[1], up[2]);
			build_rotmatrix(mat, curr_quat);
			glMultMatrixf(&mat[0][0]);
		}
		float fMM2M = 1.f/1000.f;
		float fExtent = m_Base.GetScaleFactor();
		if (m_bLite)
			fExtent = m_Y.GetScaleFactor();
			
		// Fit to -1, 1
		glScalef(1.0f / fExtent, 1.0f / fExtent, 1.0f / fExtent);
		float fTransform[3];
		if (m_bLite)	
			m_Y.GetCenteringTransform(fTransform);
		else
			m_Base.GetCenteringTransform(fTransform);
		// Centerize object.
		glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
		if (m_bFollowNozzle)
		{
			float fLook[3] = {.025f+m_fXPos+fTransform[0] ,m_fZPos+0.02f, -0.01f};
			gluLookAt(fLook[0]+.001, fLook[1]+.003 ,fLook[2]+.08, fLook[0],fLook[1],fLook[2] ,0,1,0);
		}
		glPushMatrix();   
			glTranslatef(0,-m_fZCorr + (m_fZPos),0);
			m_Z.Draw();
			glPushMatrix();
				glTranslatef(-m_fXCorr + (m_fXPos),0,0);
			 m_Extruder.Draw();
			 if (!m_bPINDAOn)
				{
					glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE | GL_SPECULAR,fNone);
					glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,fLED);
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
					glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION, fNone);
				}
				glPushMatrix();
					glScalef(fMM2M,fMM2M,fMM2M);
					if (m_bMMU)
						m_EMMU.Draw();
					else
						m_EStd.Draw();
				glPopMatrix();

				glPushMatrix();
					m_EPFan.GetCenteringTransform(fTransform);
					if (m_bPFanOn)
						m_iPFanPos = (m_iPFanPos + 5)%360;
					glTranslatef(0.086,0.328,0.314);
					glRotatef(180-45.0,1,0,0);
					glPushMatrix();
						glRotatef((float)m_iPFanPos,0,1,0);
						m_EPFan.Draw();
					glPopMatrix();
				glPopMatrix();

				glPushMatrix();
					glScalef(fMM2M,fMM2M,fMM2M);
					if (m_bFanOn)
						m_iFanPos = (m_iFanPos + 339)%360;
					m_EFan.GetCenteringTransform(fTransform);
					glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
					glRotatef((float)m_iFanPos,1,0,0);
					glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
					
					m_EFan.Draw();
				glPopMatrix();
	
				glPushMatrix();  
					glScalef(fMM2M,fMM2M,fMM2M);
					m_EVis.GetCenteringTransform(fTransform);
					fTransform[1] +=1.5f;
					glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
					glRotatef((-36.f/28.f)*3.f*(m_fEPos*1000.f),0,0,1);
					glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
					m_EVis.Draw();
				glPopMatrix();
			glPopMatrix();
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0,0, -m_fYCorr + (m_fYPos));
			m_Y.Draw();
			glTranslatef(0.025,0.083,0.431);
			m_Sheet.Draw();
			if (m_bBedOn)
			{
				glTranslatef(0.016,0,-0.244);
				DrawLED(1,0,0);
			}
		glPopMatrix();
		m_Base.Draw();
		glPushMatrix();
			glTranslatef(0.215,0.051,0.501);
			glRotatef(-45.f,1,0,0);
			glPushMatrix();  
				glRotatef((float)m_iKnobPos,0,0,1);
				m_Knob.Draw();
			glPopMatrix();
		glPopMatrix();
		if (m_pLCD)
		{
			glPushMatrix();
				glTranslatef(0.101,0.0549,0.4925);
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
		glPushMatrix();
			glScalef(fMM2M,fMM2M,fMM2M);
			m_SDCard.Draw();
		glPopMatrix();
		if (m_bMMU)
			DrawMMU();
		glutSwapBuffers();
		m_bDirty = false;
}

void MK3SGL::DrawLED(float r, float g, float b)
{
		float fLED[4] = {r,g,b,1};
		float fNone[4] = {0,0,0,1};
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE | GL_SPECULAR,fNone);
				glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,fLED);
				glBegin(GL_QUADS);
					glVertex3f(0,0,0);
					glVertex3f(0.004,0,0);
					glVertex3f(0.004,0.003,0.003);
					glVertex3f(0,0.003,0.003);
				glEnd();
				glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION, fNone);
}

void MK3SGL::DrawMMU()
{
		float fTransform[3];
		glPushMatrix();
			m_MMUBase.GetCenteringTransform(fTransform);
			glTranslatef(0,0.3185,0.0425);
			glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
			glRotatef(-45,1,0,0);
			glTranslatef(0.13+fTransform[0],fTransform[1],fTransform[2]);
			m_MMUBase.Draw();
			glPushMatrix();
				m_MMUSel.GetCenteringTransform(fTransform);
				glTranslatef(m_fSelPos - m_fSelCorr,0.062,0.123);
				glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
				glRotatef(-90,1,0,0);					
				glRotatef(-2.5,0,1,0);					
				glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
				m_MMUSel.Draw();
			glPopMatrix();
			glPushMatrix();
				m_MMUIdl.GetCenteringTransform(fTransform);
				glTranslatef(-0.03,0.028,0.025);
				fTransform[1]=-0.071;
				fTransform[2]=-0.0929;
				glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
				glRotatef(-m_fIdlPos - m_fIdlCorr,1,0,0);					
				glRotatef(180,0,1,0);
				glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
				m_MMUIdl.Draw();
			glPopMatrix();

		glPopMatrix();
	}	


void MK3SGL::MouseCB(int button, int action, int x, int y)
{
 if (button == GLUT_LEFT_BUTTON) {
		if (action == GLUT_DOWN) {
			mouseLeftPressed = true;
			trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
		} else if (action == GLUT_UP) {
			mouseLeftPressed = false;
		}
	}
	if (button == GLUT_RIGHT_BUTTON) {
		if (action == GLUT_DOWN) {
			mouseRightPressed = true;
		} else if (action == GLUT_UP) {
			mouseRightPressed = false;
		}
	}
	if (button == GLUT_MIDDLE_BUTTON) {
		if (action == GLUT_DOWN) {
			mouseMiddlePressed = true;
		} else if (action == GLUT_UP) {
			mouseMiddlePressed = false;
		}
	}
	if (button==3 || button==4) // wheel
	{
		if (button==3)
			eye[2] += 0.05f;
		else
			eye[2] -= 0.05f;
	}
	m_bDirty = true;
}

void MK3SGL::MotionCB(int x, int y)
{
			float rotScale = 1.0f;
	float transScale = 2.0f;

	if (mouseLeftPressed) {
		trackball(prev_quat, rotScale * (2.0f * prevMouseX - width) / (float)width,
							rotScale * (height - 2.0f * prevMouseY) / (float)height,
							rotScale * (2.0f * x - width) / (float)width,
							rotScale * (height - 2.0f * y) / (float)height);

		add_quats(prev_quat, curr_quat, curr_quat);
	} else if (mouseMiddlePressed) {
		eye[0] -= transScale * (x - prevMouseX) / (float)width;
		lookat[0] -= transScale * (x - prevMouseX) / (float)width;
		eye[1] += transScale * (y - prevMouseY) / (float)height;
		lookat[1] += transScale * (y - prevMouseY) / (float)height;
	} else if (mouseRightPressed) {
		eye[2] += transScale * (y - prevMouseY) / (float)height;
		lookat[2] += transScale * (y - prevMouseY) / (float)height;
	}

	// Update mouse point
	prevMouseX = x;
	prevMouseY = y;
	m_bDirty = true;
}
