/*
	Prusa_MMU2.cpp - Standalone MMU2 sim for MK404
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

#include "Prusa_MMU2.h"
#include "GLHelper.h"
#include "MK3SGL.h"

std::pair<int,int> Prusa_MMU2::GetWindowSize()
{
	return {125,50};
}


void Prusa_MMU2::Draw()
{
	glScalef(50.F/35.F,4,1);
	MMU2::Draw(static_cast<float>(GetWindowSize().second));
	m_gl.OnDraw();
}

