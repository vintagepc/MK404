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
#include "MK3S_Lite.h"
#include "gsl-lite.hpp"
#include <memory>

class MK3S_Full: public MK3S_Lite
{
	public:
		explicit MK3S_Full(bool bMMU);

		inline void SetNozzleCam(bool bOn) override { m_pE->SetSubobjectVisible(0,!bOn); }

		inline void OnLoadComplete() override {};

		inline float GetScaleFactor() override { return m_pBaseObj->GetScaleFactor();};

		inline void GetBaseCenter(gsl::span<float>fTrans) override {m_pBaseObj->GetCenteringTransform(fTrans);}
};
