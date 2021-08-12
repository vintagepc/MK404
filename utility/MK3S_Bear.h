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

#pragma once

#include "GLObj.h"
#include "OBJCollection.h"
#include "gsl-lite.hpp"
#include <GL/glew.h>
#include <memory>


class MK3S_Bear: public OBJCollection
{
	public:
		explicit MK3S_Bear(bool /*bMMU*/);

		void SetupLighting() override;

		inline void ApplyLCDTransform() override { glTranslatef(-0.051000, 0.203000, 0.045); }

		inline void ApplyPLEDTransform() override {glTranslatef(-0.201000, -0.062000, -0.45);};

		inline void ApplyBedLEDTransform() override
		{
			glTranslatef(-0.109000, 0.238000, -0.412998);
			glRotatef(-90,1,0,0);
		};

		inline void ApplyPrintTransform() override { glTranslatef(-0.131000, 0.236000, 0.173000);};

		inline float GetScaleFactor() override { return m_pBaseObj->GetScaleFactor();};

		inline void SetNozzleCam(bool bOn) override { m_pE->SetSubobjectVisible(89,!bOn); }

		void GetBaseCenter(gsl::span<float> fTrans) override;

		void GetNozzleCamPos(gsl::span<float> fPos) override;


		void DrawKnob(int iRotation) override;

		void DrawEFan(int iRotation) override;

		void DrawPFan(int iRotation) override;

		void DrawEVis(float fEPos) override;

		std::shared_ptr<GLObj> m_pKnob = nullptr, m_pFan = nullptr, m_pEVis = nullptr, m_pPFan = nullptr, m_pE = nullptr, m_pPFS = nullptr;

};
