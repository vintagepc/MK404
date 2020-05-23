/*
	GLPrint.cpp - Object responsible for print visualization

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

#include "GLPrint.h"
#include "GL/glew.h"

GLPrint::GLPrint()
{	
	m_vCoords.push_back(std::make_tuple(20,-3,0));
	m_vCoords.push_back(std::make_tuple(60,-3,0));
	m_vCoords.push_back(std::make_tuple(100,-3,0));
	m_vCoords.push_back(std::make_tuple(150,150,0));
}

void GLPrint::Draw()
{
	float fColor[4] = {1,0,0,1};
	glLineWidth(1.0);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,fColor);
	glBegin(GL_LINE_STRIP);
		for (int i=0; i<m_vCoords.size(); i++)
			glVertex3f(std::get<0>(m_vCoords[i]),std::get<2>(m_vCoords[i]),std::get<1>(m_vCoords[i]));
	glEnd();
}