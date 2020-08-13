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
#include "MK3S_Lite.h"

using namespace std;


class MK3S_Full: public MK3S_Lite
{
	public:
		MK3S_Full(bool bMMU):MK3S_Lite(bMMU)
		{
			float fXCorr = -0.044;
			// float fYCorr = -0.141;
			float fZCorr = -0.210;
			SetName("Full");
			AddObject(ObjClass::Z, "assets/Z_AXIS.obj", 0,fZCorr,0);
			if (bMMU)
				AddObject(ObjClass::X, "assets/E_MMU.obj",fXCorr,fZCorr,0,MM_TO_M)->SetKeepNormalsIfScaling(true);
			else
				AddObject(ObjClass::X, "assets/E_STD.obj",fXCorr,fZCorr,0,MM_TO_M)->SetKeepNormalsIfScaling(true);
			 m_pBaseObj = AddObject(ObjClass::Fixed, "assets/Stationary.obj");
		};

		void OnLoadComplete() override {};

		inline float GetScaleFactor() override { return m_pBaseObj->GetScaleFactor();};

		virtual void GetBaseCenter(float fTrans[3]) override {m_pBaseObj->GetCenteringTransform(fTrans);}
};
