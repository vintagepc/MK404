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

static constexpr int iPrintRes = 100000; //0.1mm (meters/this)

GLPrint::GLPrint()
{	
	m_iExtrStart = {0,0,0,0};
	m_iExtrEnd =  {0,0,0,0};
	m_fExtrStart = m_fExtrEnd = {0,0,0,0};
}

void GLPrint::NewCoord(float fX, float fY, float fZ, float fE)
{
	
	int iX = fX*iPrintRes,iY = fY*iPrintRes,iZ = fZ*iPrintRes;

	// Test if the new coordinate is still collinear with the existing segment. 
	// Triangle area method. Slopes have risks of zero/inf/nan for very small deltas.
 	//Ax(By - Cy) + Bx(Cy - Ay) + Cx(Ay - By) 
	 // We don't bother with div by 2/abs since we only care if the area is 0. 
	int iArea = (m_iExtrStart[0]* ( m_iExtrEnd[2] - iY)) + (m_iExtrEnd[0]*(iY - m_iExtrStart[2])) + (iX * (m_iExtrStart[2] - m_iExtrEnd[2]));
	float fArea = (m_fExtrStart[0]* ( m_fExtrEnd[2] - fY)) + (m_fExtrEnd[0]*(fY - m_fExtrStart[2])) + (fX * (m_fExtrStart[2] - m_fExtrEnd[2]));
	if (fArea<0) fArea = -fArea;
	bool bColinear = (fArea/2)<1e-10 || abs((iArea/2)) <= 1;

	bool bSamePos = (iX == m_iExtrEnd[0]) && (iY == m_iExtrEnd[2]);
	bool bExtruding = fE>(m_fEMax-5e-5); //-0.0002); // Extruding if we are > than the highest E value previously observed
	// Trailing 0.0002 is fudge factor for LA/retraction threshold to avoid gaps at E end.

	if ((bSamePos && !bExtruding) || (bSamePos && (iZ == m_iExtrEnd[1]))) //SamePos doesn't inculde Z so we don't get inf/zero slopes on hops.
		return;

	if (fE>m_fEMax)
		m_fEMax = fE;
	else if (!m_bExtruding && !bExtruding)
	{
		// Just update segment end if we are mid non-extrusion (travel)
		// We don't need to track co-linearity if not extruding.
		bColinear = true;
	}
	if (bExtruding ^ m_bExtruding)
	{
		// Extruding condition has changed. Start a new segment. 
		if (bExtruding) // Just started extruding. Update the various pointers.
		{
			m_iExtrStart = m_iExtrEnd;
			m_fExtrStart = m_fExtrEnd;
			m_ivStart.push_back(m_fvDraw.size()/3); // Index of what we're about to add...
			//printf("New extrusion %u at index %u\n",m_ivStart.size(),m_ivStart.back());
		}
		m_fvDraw.insert(m_fvDraw.end(),m_fExtrEnd.data(), m_fExtrEnd.data()+3);
		if (!bExtruding)
		{
			m_ivCount.push_back((m_fvDraw.size()/3) - m_ivStart.back());
			//printf("Ended extrusion %u (%u vertices)\n", m_ivCount.size(), m_ivCount.back());
		}
		m_bExtruding = bExtruding;
	}
	else if (!bColinear) 
	{
		// New segment, push it onto the vertex list and update the segment count
		//printf("New segment: %d\n",m_vCoords.size());
		m_fvDraw.insert(m_fvDraw.end(),m_fExtrEnd.data(), m_fExtrEnd.data()+3);
		m_iExtrStart = m_iExtrEnd;
		m_fExtrStart = m_fExtrEnd;
	}
	// Update the end we are tracking.
	m_fExtrEnd[0] = fX;
	m_fExtrEnd[2] = fY;
	m_fExtrEnd[1] = fZ;
	m_iExtrEnd[0] = iX; 
	m_iExtrEnd[1] = iY;
	m_iExtrEnd[2] = iZ;
	//m_iExtrEnd[3] = iE;
	
}

void GLPrint::Draw()
{
	float fColor[4] = {0.8,0,0,1};
	float fG[4] = {0,0.8,0,1};
	float fY[4] = {1,1,0,1};
	float fSpec[4] = {1,1,1,1};
	glLineWidth(1.0);
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fColor);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,fSpec);
	glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,64);
//	glEnable(GL_AUTO_NORMAL);
	//glEnable(GL_NORMALIZE);
	glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 3*sizeof(float), m_fvDraw.data());
		glMultiDrawArrays(GL_LINE_STRIP,m_ivStart.data(),m_ivCount.data(), m_ivCount.size());
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fSpec);
		if (m_ivCount.size()>0)
			glDrawArrays(GL_LINE_STRIP,m_ivStart.back(),((m_fvDraw.size()/3)-m_ivStart.back())-1);
		if (m_bExtruding)
		{
			glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fY);
			glBegin(GL_LINES);
				glVertex3fv((&m_fvDraw.back())-2);
				glVertex3fv(m_fExtrEnd.data());
			glEnd();
		}
		// Uncomment for vertex debugging. 
		// glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fG);
		// glPointSize(1.0);
		// glDrawArrays(GL_POINTS,0,m_fvDraw.size()/3);
	glDisableClientState(GL_VERTEX_ARRAY);
//	glDisable(GL_AUTO_NORMAL);
	//glDisable(GL_NORMALIZE);
}