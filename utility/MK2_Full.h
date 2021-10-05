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

#pragma once

#include "GLObj.h"
#include "OBJCollection.h"
#include "gsl-lite.hpp"
#include <GL/glew.h>
#include <memory>


class MK2_Full: public OBJCollection
{
	public:
		explicit MK2_Full(bool bMMU, bool bMK25 = false);


		void OnLoadComplete() override;

		void SetupLighting() override;

		inline bool SupportsMMU() override { return true; }

		inline void ApplyLCDTransform() override { glTranslatef(0.135,0.0399,0.255); }

		inline void ApplyPLEDTransform() override {
			if (m_bMK25)
			{
				glTranslatef(-0.011000, -0.221000, -0.236000);
			}
			else
			{
				glTranslatef(-0.012000, -0.232000, -0.242000);
			}
		};

		inline void ApplyBedLEDTransform() override {glTranslatef(0.076000, 0.066000, -0.185000);};

		inline void ApplyPrintTransform() override { glTranslatef(0.057,0.066,-0.041); };

		void GetBaseCenter(gsl::span<float> fTrans) override;

		float GetScaleFactor() override { return m_pBaseObj->GetScaleFactor(); }

		void DrawKnob(int iRotation) override;

		inline void GetNozzleCamPos(gsl::span<float> fPos) override
		{
			fPos[0] = -.101f;
			fPos[1] = -0.152f;
			fPos[2] = -0.317f;
		}

		void DrawEVis(float fEPos) override;

		void DrawEFan(int iRotation) override;

		void DrawPFan(int iRotation) override;

	protected:
		bool m_bMK25 = false, m_bMMU = false;
		std::shared_ptr<GLObj> m_pKnob = nullptr, m_pEFan = nullptr, m_pPFan = nullptr, m_pEVis = nullptr, m_pE = nullptr, m_pPShroud = nullptr;

};
