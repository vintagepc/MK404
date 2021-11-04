/*
	CW1S_Lite.h - Object collection for the "lite" visuals.

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

#pragma once

#include "OBJCollection.h"
#include "gsl-lite.hpp"
#include <GL/glew.h>
#include <memory>

class GLObj;

class CW1S_Lite: public OBJCollection
{
	public:
		explicit CW1S_Lite();

		void OnLoadComplete() override;

		void SetupLighting() override;

		inline void ApplyLCDTransform() override {  glTranslatef(-0.457, 0.123, 0.717);	glRotatef(90.F,0,1,0); glRotatef(35.f,1,0,0);}

		void GetBaseCenter(gsl::span<float>fTrans) override;

		inline float GetScaleFactor() override { return 1.5F*m_pBaseObj->GetScaleFactor();};

		void DrawKnob(int iRotation) override;

		inline void GetNozzleCamPos(gsl::span<float> fPos) override
		{
			fPos[0] = -.155f;
			fPos[1] = -0.127f;
			fPos[2] = -0.024f;
		}

		void DrawEVis(float fEPos) override;

		void DrawEFan(int iRotation) override;

		void DrawPFan(int iRotation) override;

		void DrawGeneric1(uint32_t uiVal) override;
		void DrawGeneric2(uint32_t uiVal) override;
		void DrawGeneric3(uint32_t uiVal) override;

	protected:

		uint32_t m_uiLEDlast = 999;

		std::shared_ptr<GLObj> m_pKnob = nullptr, m_pEFan = nullptr, m_pPFan = nullptr, m_pEVis = nullptr, m_pTank = nullptr, m_pLid = nullptr, m_pLEDs = nullptr;

};
