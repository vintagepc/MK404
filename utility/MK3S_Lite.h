/*
	MK3S_Lite.h - Object collection for the "lite" visuals.

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

#pragma once

#include "GLObj.h"
#include "OBJCollection.h"

using namespace std;


class MK3S_Lite: public OBJCollection
{
	public:
		MK3S_Lite(bool bMMU):OBJCollection("Lite")
		{
			AddObject(ObjClass::Y, "assets/Y_AXIS.obj", 0, 0, -0.141);
			AddObject(ObjClass::Y, "assets/SSSheet.obj", 0.025,0.083,0.431 + -0.141);
			AddObject(ObjClass::Media, "assets/SDCard.obj",0,0,0,MM_TO_M)->SetKeepNormalsIfScaling(true);
			AddObject(ObjClass::X, "assets/X_AXIS.obj",-0.044,-0.210,0);
			m_pKnob = AddObject(ObjClass::Other, "assets/LCD-knobR2.obj");
			m_pPFan = AddObject(ObjClass::Other, "assets/Print-fan_rotor.obj");
			m_pEFan = AddObject(ObjClass::Other, "assets/E_Fan.obj",MM_TO_M);
			m_pEFan->SetKeepNormalsIfScaling(true);
			m_pEVis = AddObject(ObjClass::Other,"assets/Triangles.obj",MM_TO_M);
			m_pEVis->SetKeepNormalsIfScaling(true);
		};


		void OnLoadComplete() override
		{
			m_mObjs.at(ObjClass::Y).at(0)->SetAllVisible(false);
			m_mObjs.at(ObjClass::Y).at(0)->SetSubobjectVisible(2); // heatbed, sheet
			auto pExtruder = m_mObjs.at(ObjClass::X).at(0);
			pExtruder->SetAllVisible(false);
			pExtruder->SetSubobjectVisible(19); // V6
			//pExtruder->SetSubobjectVisible(20);
			pExtruder->SetSubobjectVisible(1); // PINDA
			pExtruder->SetSubobjectVisible(2);
		}

		void SetupLighting() override
		{
			float fAmb[] = {.1,.1,.1,1};
			float fSpec[] = {.4,.4,.4,.5};
			float fDiff[] = {1.5,1.5,1.5,1};
			float fPos[] = {2,2,2,0};
			glLightfv(GL_LIGHT0,GL_AMBIENT, fAmb);
			glLightfv(GL_LIGHT0,GL_SPECULAR, fSpec);
			glLightfv(GL_LIGHT0,GL_DIFFUSE, fDiff);
			glLightfv(GL_LIGHT0,GL_POSITION, fPos);
		}

		inline bool SupportsMMU() override { return true; }

		inline void ApplyLCDTransform() override { glTranslatef(0.101,0.0549,0.4925); }

		inline void ApplyPLEDTransform() override {glTranslatef(-0.044,-0.210,0.f);};

		inline void ApplyPrintTransform() override { glTranslatef(0.024,0.084,-0.281); };

		virtual void GetBaseCenter(float fTrans[3]) override
		{
			// Values stolen from the full model so we don't have to load the frame:
			fTrans[0] = -0.154;
			fTrans[1] = -0.204;
			fTrans[2] = -0.3134;
		}

		float GetScaleFactor() override { return 0.210874f; }

		virtual void DrawKnob(int iRotation) override
		{
			if (m_pKnob == nullptr)
				return;
			glPushMatrix();
				glTranslatef(0.215,0.051,0.501);
				glRotatef(-45.f,1,0,0);
				glPushMatrix();
					glRotatef((float)iRotation,0,0,1);
					m_pKnob->Draw();
				glPopMatrix();
			glPopMatrix();
		}

		inline void GetNozzleCamPos(float fPos[3]) override
		{
			fPos[0] = -.135f;
			fPos[1] = -0.13f;
			fPos[2] = -0.04f;
		}

		virtual void DrawEVis(float fEPos) override
		{
			float fTransform[3];
			glTranslatef(-0.044,-0.210,0.f);
			m_pEVis->GetCenteringTransform(fTransform);
			fTransform[1] +=.0015f;
			glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
			glRotatef((-36.f/28.f)*3.f*(fEPos*1000.f),0,0,1);
			glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
			m_pEVis->Draw();
		}

		virtual void DrawEFan(int iRotation) override
		{
			glTranslatef(-0.044,-0.210,0.f);
			float fTransform[3];
			m_pEFan->GetCenteringTransform(fTransform);
			glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
			glRotatef((float)iRotation,1,0,0);
			glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
			m_pEFan->Draw();
		}

		virtual void DrawPFan(int iRotation) override
		{
			glTranslatef(0.042,0.118,0.314);
			glRotatef(180-45.0,1,0,0);
			glPushMatrix();
				glRotatef((float)iRotation,0,1,0);
				m_pPFan->Draw();
			glPopMatrix();
		}


	protected:

		GLObj *m_pKnob = nullptr, *m_pEFan = nullptr, *m_pPFan = nullptr, *m_pEVis = nullptr;

};
