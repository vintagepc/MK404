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
#include <algorithm>

static constexpr int iPrintRes = 100000; //0.1mm (meters/this)

GLPrint::GLPrint()
{	
	Clear();
}

void GLPrint::Clear()
{
	m_iExtrStart = m_iExtrEnd = {0,0,0,0};
	m_fExtrStart = m_fExtrEnd = {0,0,0,0};
	m_ivCount.clear();
	m_ivStart.clear();
	m_fvDraw.clear();
	m_fvNorms.clear();
	m_bExtruding = false;
	m_fEMax = 0;
}

void GLPrint::NewCoord(float fX, float fY, float fZ, float fE)
{
	
	int iX = fX*iPrintRes,iY = fY*iPrintRes,iZ = fZ*iPrintRes;
	std::array<float,3> vfPos = {fX,fZ,fY};
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
			m_ivTStart.push_back(m_fvTri.size()/3);
			if (fZ>m_fCurZ)
			{
				//printf("New Z layer: %.02f\n", fZ);
				//m_fLastZ = m_fCurZ; 
				//m_fCurZ = fZ;
			}
			//printf("New extrusion %u at index %u\n",m_ivStart.size(),m_ivStart.back());
			m_fvDraw.insert(m_fvDraw.end(),m_fExtrEnd.data(), m_fExtrEnd.data()+3);
			// Add a temporary normal vertex
			float fCross[3], fA[3],fB[3]= {0,-0.002,0};
			std::transform(vfPos.begin(), vfPos.end(), m_fExtrEnd.data(), fA, std::minus<float>());
			CrossProduct(fA,fB,fCross);
			Normalize(fCross);
			m_fvNorms.insert(m_fvNorms.end(), fCross, fCross+3);

		}
		// m_fvTri.push_back(m_fExtrEnd[0]);
		// m_fvTri.push_back(m_fExtrEnd[1]-0.0002);
		// m_fvTri.push_back(m_fExtrEnd[2]);
		// m_fvTri.insert(m_fvTri.end(),m_fExtrEnd.data(), m_fExtrEnd.data()+3);

		if (!bExtruding)
		{
			m_ivCount.push_back((m_fvDraw.size()/3) - m_ivStart.back());
			// m_ivTCount.push_back((m_fvTri.size()/3) - m_ivTStart.back());
			//printf("Ended extrusion %u (%u vertices)\n", m_ivCount.size(), m_ivCount.back());
		}
		m_bExtruding = bExtruding;
	}
	else if (!bColinear) 
	{
		// New segment, push it onto the vertex list and update the segment count
		//printf("New segment: %d\n",m_vCoords.size());
		m_fvDraw.insert(m_fvDraw.end(),m_fExtrEnd.data(), m_fExtrEnd.data()+3);
		// First, update the previous normal with the new vertex info.
		// TODO: fB really should be pointing at the nearest vertex on the layer below, but that's a lot of coordinate
		// that's going to be disposable once this changes to a geometry or normal shader instead of the current implemetnation
		// so I'm going to leave it as is for now.
		float fCross[3], fA[3],fB[3]  = {0,-0.002,0};
		float *pfPrev = (&m_fvNorms.back()) - 2;
		std::transform(pfPrev, pfPrev+3, vfPos.data(), fA, std::minus<float>()); // Length from p->curr
		CrossProduct(fA,fB,fCross);
		Normalize(fCross);
		pfPrev[0] +=fCross[0]; pfPrev[1] += fCross[1]; pfPrev[2] += fCross[2];
		pfPrev[0]/=2.f; pfPrev[1]/=2.f; pfPrev[2]/=2.f;
		// And then append the new temporary end one. 
		std::transform(vfPos.begin(), vfPos.end(), m_fExtrEnd.data(), fA, std::minus<float>());
		CrossProduct(fA,fB,fCross);
		Normalize(fCross);
		m_fvNorms.insert(m_fvNorms.end(), fCross, fCross+3);
		// m_fvTri.push_back(m_fExtrEnd[0]);
		// m_fvTri.push_back(m_fExtrEnd[1]-0.0002);
		// m_fvTri.push_back(m_fExtrEnd[2]);.
		// m_fvTri.insert(m_fvTri.end(),m_fExtrEnd.data(), m_fExtrEnd.data()+3);
		m_iExtrStart = m_iExtrEnd;
		m_fExtrStart = m_fExtrEnd;
	}
	// Update the end we are tracking.
	m_fExtrEnd[0] = fX;
	m_fExtrEnd[2] = fY;
	m_fExtrEnd[1] = fZ;
	m_iExtrEnd[0] = iX; 
	m_iExtrEnd[2] = iY;
	m_iExtrEnd[1] = iZ;
	//m_iExtrEnd[3] = iE;
	
}

void GLPrint::Draw()
{
	float fColor[4] = {0.8,0,0,1};
	float fG[4] = {0,0.5,0,1};
	float fY[4] = {1,1,0,1};
	float fK[4] = {0,0,0,1};
	float fSpec[4] = {1,1,1,1};
	glLineWidth(1.0);
	
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,fSpec);
	glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,64);
	//glEnable(GL_AUTO_NORMAL);
	//glEnable(GL_NORMALIZE);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fK);
		glVertexPointer(3, GL_FLOAT, 3*sizeof(float), m_fvTri.data());
		// glMultiDrawArrays(GL_TRIANGLE_STRIP,m_ivTStart.data(),m_ivTCount.data(), m_ivTCount.size());
		//glNormal3f(0,1,0);
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fColor);
		glVertexPointer(3, GL_FLOAT, 3*sizeof(float), m_fvDraw.data());
		glNormalPointer(GL_FLOAT, 3*sizeof(float), m_fvNorms.data());
		glMultiDrawArrays(GL_LINE_STRIP,m_ivStart.data(),m_ivCount.data(), m_ivCount.size());
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fSpec);
		if (m_ivCount.size()>0)
			glDrawArrays(GL_LINE_STRIP,m_ivStart.back(),((m_fvDraw.size()/3)-m_ivStart.back())-1);
		if (m_bExtruding && m_fvDraw.size() >0) 
		{
			glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fY);
			glBegin(GL_LINES);
				glVertex3fv((&m_fvDraw.back())-2);
				glVertex3fv(m_fExtrEnd.data());
			glEnd();
		}
		// Uncomment for vertex debugging. 
		 glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fG);
		// glPointSize(1.0);
		// glDrawArrays(GL_POINTS,0,m_fvDraw.size()/3);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	// Normal visualization for debugging, draws the last 10.
	// glBegin(GL_LINES);
	// 	int iStart = 0;
	// 	if (m_fvNorms.size()>30)
	// 		iStart = m_fvNorms.size() - 30;
	// 	for (int i=iStart; i<m_fvNorms.size(); i+=3)
	// 	{
	// 		glVertex3fv(m_fvDraw.data()+i);
	// 		glVertex3fv(m_fvNorms.data()+i);
	// 	}

	// glEnd();
	//glDisable(GL_AUTO_NORMAL);
	//glDisable(GL_NORMALIZE);
}