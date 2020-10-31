/*
	MK2_Full.h - Object collection for the "lite" visuals.

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

#include "MK2_Full.h"
#include "GLObj.h"
#include "OBJCollection.h"
#include <map>              // for map
#include <string>           // for string
#include <vector>           // for vector


MK2_Full::MK2_Full(bool bMMU, bool bMK25):OBJCollection("MK2Full"), m_bMK25(bMK25), m_bMMU(bMMU)
{
	auto pY = AddObject(ObjClass::Y, "assets/MK2_Y.obj",0,0,-0.098);
	pY->ForceDissolveTo1(true);
	pY->SetReverseWinding(true);

	AddObject(ObjClass::Media, "assets/SDCard.obj",0.034000, -0.015000, -0.238000,MM_TO_M)->SetKeepNormalsIfScaling(true);
	auto pZ = AddObject(ObjClass::Z, "assets/MK2_Z.obj",0,-0.099,0);
	pZ->ForceDissolveTo1(true);
	pZ->SetReverseWinding(true);

	if (bMK25) // For 2.5 and 2.5S
	{
		AddObject(ObjClass::PrintSurface, "assets/SSSheet.obj", 0.060,0.066,0.431 + -0.373);
		m_pE = AddObject(ObjClass::X, "assets/X_AXIS.obj",-0.011,-0.221,-0.236);
		AddObject(ObjClass::X, "assets/E_STD.obj",-0.011,-0.221,-0.2360,MM_TO_M)->SetKeepNormalsIfScaling(true);
		m_pEFan = AddObject(ObjClass::Other, "assets/E_Fan.obj",MM_TO_M);
		m_pEFan->SetKeepNormalsIfScaling(true);
	}
	else
	{
		if (bMMU)
		{
			auto pTmp = AddObject(ObjClass::Fixed, "assets/MK2MMU_Base.obj");
			pTmp->ForceDissolveTo1(true);
			pTmp->SetReverseWinding(true);
			m_pE = AddObject(ObjClass::X, "assets/MK2MMU_E.obj",-.125,-0.099,0);
		}
		else
		{
			m_pE = AddObject(ObjClass::X, "assets/MK2_E.obj",-.125,-0.099,0);
		}
		m_pE->ForceDissolveTo1(true);
		m_pE->SetReverseWinding(true);
		m_pEFan = AddObject(ObjClass::Other, "assets/MK2_EFan.obj",MM_TO_M);
		m_pEFan->SetKeepNormalsIfScaling(true);
		m_pEFan->ForceDissolveTo1(true);
	}
	m_pKnob = AddObject(ObjClass::Other, "assets/LCD-knobR2.obj");
	m_pKnob->ForceDissolveTo1(true);
	m_pKnob->SetReverseWinding(true);
	m_pPFan = AddObject(ObjClass::Other, "assets/Print-fan_rotor.obj");
	m_pPShroud = AddObject(ObjClass::Other, "assets/bear21_mk3s_simulator_print_fan.obj", CM_TO_M);
	m_pPShroud->SetSwapMode(GLObj::SwapMode::YMINUSZ);
	m_pEVis = AddObject(ObjClass::Other,"assets/Triangles.obj",MM_TO_M);
	m_pEVis->SetKeepNormalsIfScaling(true);
	m_pBaseObj = AddObject(ObjClass::Fixed, "assets/MK2_Base.obj");
	m_pBaseObj->ForceDissolveTo1(true);
	m_pBaseObj->SetReverseWinding(true);


};

void MK2_Full::OnLoadComplete()
{
	// m_mObjs.at(ObjClass::Y).at(0)->SetAllVisible(false);
	// m_mObjs.at(ObjClass::Y).at(0)->SetSubobjectVisible(2); // heatbed, sheet
	// auto pExtruder = m_mObjs.at(ObjClass::X).at(0);
	// pExtruder->SetAllVisible(false);
	// pExtruder->SetSubobjectVisible(19); // V6
	// //pExtruder->SetSubobjectVisible(20);
	// pExtruder->SetSubobjectVisible(1); // PINDA
	// pExtruder->SetSubobjectVisible(2);
}

void MK2_Full::SetupLighting()
{
	float fAmb[] = {.1,.1,.1,1};
	float fSpec[] = {.4,.4,.4,.5};
	float fDiff[] = {1.5,1.5,1.5,1};
	float fPos[] = {2,2,2,0};
	glLightfv(GL_LIGHT0,GL_AMBIENT, 	static_cast<float*>(fAmb));
	glLightfv(GL_LIGHT0,GL_SPECULAR, 	static_cast<float*>(fSpec));
	glLightfv(GL_LIGHT0,GL_DIFFUSE, 	static_cast<float*>(fDiff));
	glLightfv(GL_LIGHT0,GL_POSITION, 	static_cast<float*>(fPos));
}

void MK2_Full::GetBaseCenter(gsl::span<float> fTrans)
{
	// Values stolen from the full model so we don't have to load the frame:
	fTrans[0] = -0.154;
	fTrans[1] = -0.204;
	fTrans[2] = -0.3134;
}

void MK2_Full::DrawKnob(int iRotation)
{
	if (m_pKnob != nullptr)
	{
		glPushMatrix();
			glTranslatef(0.249,0.036,0.263);
			glRotatef(-45.f,1,0,0);
			glPushMatrix();
				glRotatef(static_cast<float>(iRotation),0,0,1);
				m_pKnob->Draw();
			glPopMatrix();
		glPopMatrix();
	}
}

void MK2_Full::DrawEVis(float fEPos)
{
	if (m_bMMU) return;

	float fTransform[3];
	if (m_bMK25)
	{
		glTranslatef(-0.011000, -0.221000, -0.236000);
	}
	else
	{
		glTranslatef(-0.010000, -0.221000, -0.243000);
	}
	m_pEVis->GetCenteringTransform(fTransform);
	fTransform[1] +=.0015f;
	glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
	glRotatef((-36.f/28.f)*6.f*(fEPos*1000.f),0,0,1);
	glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
	m_pEVis->Draw();
}

void MK2_Full::DrawEFan(int iRotation)
{
	if (!m_bMK25)
	{
	 	glScalef(0.75,0.75,0.75);
		glTranslatef(0.0037, -0.193, -0.226);
	}
	else
	{
		glTranslatef(-0.0117, -0.221, -0.236);
	}

	float fTransform[3];
	m_pEFan->GetCenteringTransform(fTransform);
	glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
	glRotatef(static_cast<float>(iRotation),1,0,0);
	glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
	m_pEFan->Draw();
}

void MK2_Full::DrawPFan(int iRotation)
{
	if (m_bMK25)
	{
		glTranslatef(.047, .105, .079);
		glRotatef(180-45.0,1,0,0);
	}
	else
	{
		glTranslatef(.041, .102, .057);
		glPushMatrix();
			glRotatef(90,0,1,0);
			m_pPShroud->Draw();
		glPopMatrix();
		glRotatef(90.0,1,0,0);
	}
	glPushMatrix();
		glTranslatef(.028,0,0);
		glRotatef(static_cast<float>(360-iRotation),0,1,0);
		m_pPFan->Draw();
	glPopMatrix();
}
