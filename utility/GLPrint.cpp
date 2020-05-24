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
	m_vCoords.push_back(std::array<float,4>{0,0,0,0});
	m_pfSegStart = &m_vCoords[0];
}

void GLPrint::NewCoord(float fX, float fY, float fZ, float fE)
{
	// Test if the new coordinate is still collinear with the existing segment. 
	// Triangle area method. Slopes have risks of zero/inf/nan for very small deltas.
 	//Ax(By - Cy) + Bx(Cy - Ay) + Cx(Ay - By) 
	 // We don't bother with div by 2/abs since we only care if the area is 0. 
	const std::array<float,4> fStart = *m_pfSegStart;
	float fArea = (fStart[0]* ( m_fSegEnd[1] - fY)) + (m_fSegEnd[0]*(fY - fStart[1])) + (fX * (fStart[1] - m_fSegEnd[1]));
	// float fSlope1 = ((fY-fStart[1])/(fX-fStart[0]));
	// float fSlope2 = ((fY-m_fSegEnd[1])/(fX-m_fSegEnd[0]));
	// float fSlope3 = ((m_fSegEnd[1]-fStart[1])/(m_fSegEnd[0]-fStart[0]));
	bool bSamePos = (fX == m_fSegEnd[0]) && (fY == m_fSegEnd[1]);
	bool bExtruding = !(fE<m_fSegEnd[3]) && fE>m_fERetr; // Extruding if we are > than last retraction event and also moving +

	if ((bSamePos && !bExtruding) || (bSamePos && (fZ == m_fSegEnd[2]))) //SamePos doesn't inculde Z so we don't get inf/zero slopes on hops.
		return;
	bool bCollinear =  fArea < 1E-8; // Float rounding err, this should be satisfactory... // (fSlope1==fSlope2) && (fSlope2 == fSlope3);

	bool bNewSegment = false;
	if (bExtruding ^ m_bExtruding)
	{
		// Extruding condition has changed. Start a new segment. 
		bNewSegment = true;
		if (!bExtruding) // Just stopped extruding. Store coord.
		{
			//printf("Seg end\n");
			m_fERetr = m_fSegEnd[3];
		}
		else
			//printf("Seg start\n");

		m_bExtruding = bExtruding;

	}
	if (!bCollinear || bNewSegment) 
	{
		// New segment, push it onto the vertex queue and reset the start.
		//printf("New segment: %d\n",m_vCoords.size());
		m_vCoords.push_back(m_fSegEnd);	
		m_pfSegStart = &m_vCoords.back(); // Update segment start.
	}
	// If it's the same segment just update the end we are tracking.
	m_fSegEnd[0] = fX; 
	m_fSegEnd[1] = fY;
	m_fSegEnd[2] = fZ;
	m_fSegEnd[3] = fE;
	
}

void GLPrint::Draw()
{
	float fColor[4] = {0.5,0,0,1};
	float fSpec[4] = {1,1,1,1};
	glLineWidth(1.0);
	// glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fColor);
	// glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,fSpec);
	// glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,64);
	glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_AUTO_NORMAL);
	glBegin(GL_LINE_STRIP);
		for (int i=0; i<m_vCoords.size(); i++)
		{
			if (i<m_vCoords.size()-1 && m_vCoords[i][3]<m_vCoords[i+1][3])
				glColor4f(0.8,0,0,1);
			else
			{
				// End of extr. segment, draw the point again so the line ends cleanly, then change colour.
				glColor4f(0.5,0,0,1); // add some definition with a slight colour change
				glVertex3f(m_vCoords[i][0],m_vCoords[i][2],m_vCoords[i][1]); 
				glColor4f(0,1,0,0);
			}
			glVertex3f(m_vCoords[i][0],m_vCoords[i][2],m_vCoords[i][1]);
		}
		// Also draw to the current position, it's not on the segment queue yet.
		glColor4f(1,1,1,1);
		glVertex3f(m_fSegEnd[0], m_fSegEnd[2], m_fSegEnd[1]);
	glEnd();
	glDisable(GL_AUTO_NORMAL);
	glDisable(GL_COLOR_MATERIAL);
}