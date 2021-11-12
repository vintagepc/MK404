/*
	Prusa_CW1S.cpp - Printer definition for the CW1S
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

#include "Prusa_SL1_Ctl.h"
#include "GLHelper.h"
#include <GL/glew.h>		// NOLINT must come before freeglut
#include <GL/freeglut_std.h>  // for GLUT_DOWN, GLUT_LEFT_BUTTON, GLUT_RIGHT...


std::pair<int,int> Prusa_SL1_Ctl::GetWindowSize()
{
	return {125,30};
}


void Prusa_SL1_Ctl::Draw()
{
	glScalef(50.F/35.F,4,1);
	SL1::Draw();
	m_gl.OnDraw();
}

