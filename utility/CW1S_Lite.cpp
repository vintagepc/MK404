/*
	CWS1_Lite.cpp - Object collection for the "lite" visuals.

	Copyright 2021 VintagePC <https://github.com/vintagepc/>

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

#include "CW1S_Lite.h"
#include "GLObj.h"
#include "OBJCollection.h"
#include <map>              // for map
#include <string>           // for string
#include <vector>           // for vector


CW1S_Lite::CW1S_Lite():OBJCollection("CW1S lite")
{
	m_pEVis = AddObject(ObjClass::Other, "assets/CW1S_Platform.obj",MM_TO_M);
	m_pEVis->ForceDissolveTo1(true);
	m_pEVis->SetReverseWinding(true);
	m_pKnob = AddObject(ObjClass::Other, "assets/CW1_Knob.obj", MM_TO_M);
	m_pKnob->ForceDissolveTo1(true);
	m_pKnob->SetReverseWinding(true);
	m_pPFan = AddObject(ObjClass::Other, "assets/CW1_RFan.obj", MM_TO_M);
	m_pPFan->ForceDissolveTo1(true);
	m_pPFan->SetReverseWinding(true);
	m_pEFan = AddObject(ObjClass::Other, "assets/CW1_HFan.obj",MM_TO_M);
	m_pEFan->ForceDissolveTo1(true);
	m_pEFan->SetReverseWinding(true);
	m_pTank = AddObject(ObjClass::Other, "assets/CW1_Tank.obj", MM_TO_M);
	m_pTank->ForceDissolveTo1(true);
	m_pTank->SetReverseWinding(true);
	m_pBaseObj = AddObject(ObjClass::Fixed, "assets/CW1_Base.obj",MM_TO_M);
	m_pBaseObj->ForceDissolveTo1(true);
	m_pBaseObj->SetReverseWinding(true);
	m_pLid = AddObject(ObjClass::Other, "assets/CW1_Lid.obj",MM_TO_M);
	m_pLid->ForceDissolveTo1(true);
	m_pLid->SetReverseWinding(true);
	m_pTank = AddObject(ObjClass::Other, "assets/CW1_Tank.obj",MM_TO_M);
	m_pTank->ForceDissolveTo1(true);
	m_pTank->SetReverseWinding(true);
	m_pLEDs = AddObject(ObjClass::Other, "assets/CW1_LEDs.obj",MM_TO_M);
	m_pLEDs->ForceDissolveTo1(true);
	m_pLEDs->SetReverseWinding(true);
};

void CW1S_Lite::OnLoadComplete()
{
	m_pBaseObj->SetAllVisible(false);
	m_pEVis->SetAllVisible(false);
	m_pEVis->SetSubobjectVisible(2, true);
	m_pLid->SetAllVisible(false);
	m_pLid->SetSubobjectVisible(0,true);
}

void CW1S_Lite::SetupLighting()
{
	float fAmb[] = {.7,.7,.7,1};
	float fSpec[] = {.4,.4,.4,.5};
	float fDiff[] = {1,1,1,1};
	float fPos[] = {2,2,2,0};
	glLightfv(GL_LIGHT0,GL_AMBIENT, 	static_cast<float*>(fAmb));
	glLightfv(GL_LIGHT0,GL_SPECULAR, 	static_cast<float*>(fSpec));
	glLightfv(GL_LIGHT0,GL_DIFFUSE, 	static_cast<float*>(fDiff));
	glLightfv(GL_LIGHT0,GL_POSITION, 	static_cast<float*>(fPos));
}

void CW1S_Lite::GetBaseCenter(gsl::span<float> fTrans)
{
	// Values stolen from the full model so we don't have to load the frame:
	fTrans[0] = 0.670;
	fTrans[1] = -0.273;
	fTrans[2] = -0.670;
	// Cheat a little bit and sneak in the initial rotation here:
	glRotatef(-90,0,1,0);
}

void CW1S_Lite::DrawKnob(int iRotation)
{
	if (m_pKnob != nullptr)
	{
		float fTransform[3];
		m_pKnob->GetCenteringTransform(fTransform);
		glPushMatrix();
			glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
			glRotatef(static_cast<float>(iRotation),1,0,0);
			glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
			m_pKnob->Draw();
		glPopMatrix();
	}
}

void CW1S_Lite::DrawEVis(float fEPos)
{
	float fTransform[3];
	glTranslatef(-0.009000, -0.010000, 0.000000);
	m_pEVis->GetCenteringTransform(fTransform);
	glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
	glRotatef((-36.f/28.f)*6.f*(fEPos*1000.f),0,1,0);
	glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
	m_pEVis->Draw();
}

void CW1S_Lite::DrawEFan(int iRotation)
{
	float fTransform[3];
	m_pEFan->GetCenteringTransform(fTransform);
	glPushMatrix();
		glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
		glRotatef(static_cast<float>(iRotation),1,0,0);
		glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
		m_pEFan->Draw();
	glPopMatrix();
}

void CW1S_Lite::DrawPFan(int iRotation)
{
	float fTransform[3];
	m_pPFan->GetCenteringTransform(fTransform);
	glPushMatrix();
		glTranslatef (-fTransform[0], -fTransform[1] , -fTransform[2]);
		glRotatef(static_cast<float>(iRotation),1,0,0);
		glTranslatef (fTransform[0], fTransform[1] , fTransform[2]);
		m_pPFan->Draw();
	glPopMatrix();
}


void CW1S_Lite::DrawGeneric3(uint32_t uiVal)
{
	if (uiVal)
	{
		float fTrans[3];
		glTranslatef(-0.169000, 0.264000, -0.000000);
		m_pLid->GetCenteringTransform(fTrans);
		glTranslatef(-fTrans[0],-fTrans[1],-fTrans[2]);
		glRotatef(180,0,0,1);
		glTranslatef(fTrans[0],fTrans[1],fTrans[2]);
	}
	m_pLid->Draw();
};

void CW1S_Lite::DrawGeneric2(uint32_t uiVal)
{
	if (!uiVal)
	{
		m_pTank->Draw();
	}
};

void CW1S_Lite::DrawGeneric1(uint32_t uiVal)
{
	if (uiVal != m_uiLEDlast)
	{
		if (uiVal)
		{
			m_pLEDs->SwapMaterial(0,3);
			m_pBaseObj->SwapMaterial(25, 26);
		}
		else
		{
			m_pLEDs->SwapMaterial(3,0);
			m_pBaseObj->SwapMaterial(26, 25);
		}
		m_uiLEDlast = uiVal;
	}
	m_pLEDs->Draw();
};
