/*
	MK3S_Full.h - Object collection for the standard visuals.

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

#include "MK3S_Bear.h"
#include "GLObj.h"
#include "OBJCollection.h"
#include <string>

MK3S_Bear::MK3S_Bear(bool /*bMMU*/):OBJCollection("Bear")
{
	AddObject(ObjClass::Z, "assets/bear21_mk3s_simulator_x-axis.obj",0,-.127,0,CM_TO_M)->SetSwapMode(GLObj::SwapMode::YMINUSZ);
	m_pE = AddObject(ObjClass::X, "assets/bear21_mk3s_simulator_e-axis_w-hotend-fan.obj",-0.130-0.182000, 0.318000, -0.186000,CM_TO_M);
	m_pE->SetSwapMode(GLObj::SwapMode::YMINUSZ);
	m_pPFS = AddObject(ObjClass::Other, "assets/bear21_mk3s_simulator_print_fan.obj", CM_TO_M);
	m_pPFS->SetSwapMode(GLObj::SwapMode::YMINUSZ);
	AddObject(ObjClass::Y, "assets/bear21_mk3s_simulator_y-axis.obj",0,0,-.1175,CM_TO_M)->SetSwapMode(GLObj::SwapMode::YMINUSZ);
	m_pBaseObj = AddObject(ObjClass::Fixed, "assets/bear21_mk3s_simulator_frame.obj", CM_TO_M);
	m_pBaseObj->SetSwapMode(GLObj::SwapMode::YMINUSZ);
	m_pKnob = AddObject(ObjClass::Other, "assets/bear21_mk3s_simulator_lcd_knob.obj",CM_TO_M);
	m_pFan = AddObject(ObjClass::Other, "assets/bear21_mk3s_simulator_hotend-fan-blade.obj",CM_TO_M);
	SetMaterialMode(GL_AMBIENT_AND_DIFFUSE); // These first ones only have Kd.
	AddObject(ObjClass::PrintSurface, "assets/SSSheet.obj",-0.127000,0.236000,-0.170000);
	AddObject(ObjClass::Media, "assets/SDCard.obj",-0.157000, 0.147000, -0.448998,MM_TO_M);
	m_pEVis = AddObject(ObjClass::Other,"assets/Triangles.obj",MM_TO_M);
	m_pPFan = AddObject(ObjClass::Other, "assets/Print-fan_rotor.obj");
};

void MK3S_Bear::SetupLighting()
{
	float fNone[] = {0,0,0,1};
	float fWhite[] = {1,1,1,1};

	float fPos[] = {2,-2,-2,0};
	glLightfv(GL_LIGHT0,GL_AMBIENT, 	static_cast<float*>(fNone));
	glLightfv(GL_LIGHT0,GL_SPECULAR, 	static_cast<float*>(fWhite));
	glLightfv(GL_LIGHT0,GL_DIFFUSE, 	static_cast<float*>(fWhite));
	glLightfv(GL_LIGHT0,GL_POSITION, 	static_cast<float*>(fPos));
}

void MK3S_Bear::GetBaseCenter(gsl::span<float> fTrans)
{
	m_pBaseObj->GetCenteringTransform(fTrans);
	float fTmp = fTrans[1];
	fTrans[1] = fTrans[2];
	fTrans[2] = -fTmp;
};

void MK3S_Bear::GetNozzleCamPos(gsl::span<float> fPos)
{
	fPos[0] = -0.131f;
	fPos[1] = -0.11f;
	fPos[2] = -0.054f;
}

void MK3S_Bear::DrawKnob(int iRotation)
{
	if (m_pKnob == nullptr)
	{
		return;
	}
	glPushMatrix();
		glTranslatef(0.060,0.197,0.054);
		glRotatef(-45.f,1,0,0);
		glPushMatrix();
			glRotatef(static_cast<float>(iRotation),0,0,1);
			m_pKnob->Draw();
		glPopMatrix();
	glPopMatrix();
}

void MK3S_Bear::DrawEFan(int iRotation)
{
	glTranslatef(-0.153000, 0.252000, -0.150000);
	glRotatef(90,0,1,0);
	float fTransform[3];
	m_pFan->GetCenteringTransform(fTransform);
	glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
	glRotatef(static_cast<float>(iRotation),0,0,1);
	glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
	m_pFan->Draw();
}

void MK3S_Bear::DrawPFan(int iRotation)
{
	glTranslatef(-0.11100, 0.2710, -0.148000);
	glPushMatrix();
		glTranslatef(-0.03000, 0.0, -0.002000);
		glRotatef(90,0,1,0);
		m_pPFS->Draw();
	glPopMatrix();
	glRotatef(90,1,0,0);
	glPushMatrix();
		glRotatef(static_cast<float>(iRotation),0,1,0);
		m_pPFan->Draw();
	glPopMatrix();
}

void MK3S_Bear::DrawEVis(float fEPos)
{
	glTranslatef(-0.201000, -0.053000, -0.451998);
	float fTransform[3];
	m_pEVis->GetCenteringTransform(fTransform);
	fTransform[1] +=.0015f;
	glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
	glRotatef((-36.f/28.f)*6.f*(fEPos*1000.f),0,0,1);
	glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
	m_pEVis->Draw();
}
