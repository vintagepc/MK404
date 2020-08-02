/*
	MK3S_Full.h - Object collection for the standard visuals.

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

#pragma once

#include "GLObj.h"
#include "OBJCollection.h"

using namespace std;


class MK3S_Bear: public OBJCollection
{
	public:
		MK3S_Bear(bool bMMU):OBJCollection("Bear")
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

		void SetupLighting() override
		{
			float fNone[] = {0,0,0,1};
			float fWhite[] = {1,1,1,1};

			float fPos[] = {2,-2,-2,0};
			glLightfv(GL_LIGHT0,GL_AMBIENT, fNone);
			glLightfv(GL_LIGHT0,GL_SPECULAR, fWhite);
			glLightfv(GL_LIGHT0,GL_DIFFUSE, fWhite);
			glLightfv(GL_LIGHT0,GL_POSITION, fPos);
		}

		inline bool SupportsMMU() override { return false; }

		inline void ApplyLCDTransform() override { glTranslatef(-0.051000, 0.203000, 0.045); }

		inline void ApplyPLEDTransform() override {glTranslatef(-0.201000, -0.062000, -0.45);};

		inline void ApplyPrintTransform() override { glTranslatef(-0.131000, 0.236000, 0.173000);};

		inline float GetScaleFactor() override { return m_pBaseObj->GetScaleFactor();};

		inline void SetNozzleCam(bool bOn) { m_pE->SetSubobjectVisible(100,!bOn); }

		virtual void GetBaseCenter(float fTrans[3]) override
		{
			m_pBaseObj->GetCenteringTransform(fTrans);
			float fTmp = fTrans[1];
			fTrans[1] = fTrans[2];
			fTrans[2] = -fTmp;
		};

		virtual void GetNozzleCamPos(float fPos[3]) override
		{
			fPos[0] = -0.131f;
			fPos[1] = -0.11f;
			fPos[2] = -0.054f;
		}


		virtual void DrawKnob(int iRotation) override
		{
			if (m_pKnob == nullptr)
				return;
			glPushMatrix();
				glTranslatef(0.060,0.197,0.054);
				glRotatef(-45.f,1,0,0);
				glPushMatrix();
					glRotatef((float)iRotation,0,0,1);
					m_pKnob->Draw();
				glPopMatrix();
			glPopMatrix();
		}

		virtual void DrawEFan(int iRotation) override
		{
			glTranslatef(-0.153000, 0.252000, -0.150000);
			glRotatef(90,0,1,0);
			float fTransform[3];
			m_pFan->GetCenteringTransform(fTransform);
			glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
			glRotatef((float)iRotation,0,0,1);
			glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
			m_pFan->Draw();
		}

		virtual void DrawPFan(int iRotation) override
		{
			glTranslatef(-0.11100, 0.2710, -0.148000);
			glPushMatrix();
				glTranslatef(-0.03000, 0.0, -0.002000);
				glRotatef(90,0,1,0);
				m_pPFS->Draw();
			glPopMatrix();
			glRotatef(90,1,0,0);
			glPushMatrix();
				glRotatef((float)iRotation,0,1,0);
				m_pPFan->Draw();
			glPopMatrix();
		}

		virtual void DrawEVis(float fEPos) override
		{
			glTranslatef(-0.201000, -0.053000, -0.451998);
			float fTransform[3];
		 	m_pEVis->GetCenteringTransform(fTransform);
		 	fTransform[1] +=.0015f;
		 	glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
		 	glRotatef((-36.f/28.f)*3.f*(fEPos*1000.f),0,0,1);
		 	glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
		 	m_pEVis->Draw();
		}

		GLObj *m_pKnob = nullptr, *m_pFan = nullptr, *m_pEVis = nullptr, *m_pPFan = nullptr, *m_pE = nullptr, *m_pPFS = nullptr;

};
