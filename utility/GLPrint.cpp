/*
	GLPrint.cpp - Object responsible for print visualization

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

#include "Config.h"
#include "Color.h"
#include "GLPrint.h"
#include "gsl-lite.hpp"
#include <GL/glew.h>   // for glMaterialfv, GL_FRONT_AND_BACK, glDisableClie...
#include <algorithm>   // for transform
#include <cmath>
#include <functional>  // for minus
//#include <iostream>
#include <iterator>


static void CrossProduct(const std::vector<float>&fA, const std::vector<float>&fB, gsl::span<float>fOut)
{
	fOut[0] = (fA[1]*fB[2]) - (fA[2]*fB[1]);
	fOut[1] = (fA[2]*fB[0]) - (fA[0]*fB[2]);
	fOut[2] = (fA[0]*fB[1]) - (fA[1]*fB[0]);
};

static void Normalize(gsl::span<float>fA)
{
	float fNorm = std::sqrt((fA[0]*fA[0]) + (fA[1]*fA[1]) + (fA[2]*fA[2]));
	fA[0]/=fNorm;
	fA[1]/=fNorm;
	fA[2]/=fNorm;
}

GLPrint::GLPrint(float fR, float fG, float fB):m_fColR(fR),m_fColG(fG),m_fColB(fB)
{
	Clear();
	m_bHRE = Config::Get().GetHRE();
	m_bColExt = Config::Get().GetColourE();
	m_b3DExt = Config::Get().Get3DE();
}

void GLPrint::Clear()
{
	m_uiExtrStart = m_uiExtrEnd = {{0,0,0,0}};
	m_ivCount.clear();
	m_ivStart.clear();
	m_fvDraw.clear();
	m_fvNorms.clear();
	m_bExtruding = false;
	m_bFirst = true;
}

void GLPrint::OnEStep(const uint32_t& uiE)
{
	m_uiE = uiE;
	if (m_bFirst) // First cycle/extrusion.
	{
		m_bFirst = false;
		std::lock_guard<std::mutex> lock(m_lock); // Lock out GL while updating vectors
		m_iEMax = uiE;
		m_uiExtrEnd = m_uiExtrStart = {{m_uiX,m_uiZ,m_uiY,m_uiE}};
	}
	// Test if the new coordinate is still collinear with the existing segment.
	// Triangle area method. Slopes have risks of zero/inf/nan for very small deltas.
 	//Ax(By - Cy) + Bx(Cy - Ay) + Cx(Ay - By)
	 // We don't bother with div by 2/abs since we only care if the area is 0.
	int iArea = (m_uiExtrStart[0]* ( m_uiExtrEnd[2] - m_uiY)) + (m_uiExtrEnd[0]*(m_uiY - m_uiExtrStart[2])) + (m_uiX * (m_uiExtrStart[2] - m_uiExtrEnd[2]));

	bool bColinear = iArea == 0;
	bool bSamePos = (m_uiX == m_uiExtrEnd[0]) && (m_uiY == m_uiExtrEnd[2]);
	bool bExtruding; // Extruding if we are > than the highest E value previously observed
	if (!m_b3DExt)
	{
		bExtruding = (uiE+64)>=(m_iEMax);
	}
	else
	{
		bExtruding = uiE>=m_iEMax;
	}


	auto idX = m_uiX - m_uiExtrEnd[0];
	auto idY = m_uiY - m_uiExtrEnd[2];
	auto idE = m_uiE - m_uiExtrEnd[3];
	float fDist = std::sqrt(static_cast<float>((idX*idX)+(idY*idY)));
	float fERate = 0;
	if (m_bHRE && fDist > 0)
	{
		fERate = static_cast<float>(idE)/fDist; // This is the extrusion distance to travel distance ratio.
		//std::cout << "fERate:" << std::to_string(fERate) << " dXY:" << std::to_string(fDist) << "dE:" << std::to_string(idE) << '\n';
		if (std::abs(m_fLastERate - fERate)>1e-5) // if dE is changing, mark it as non-colinear to force a segment add.
		{
			bColinear = false;
		}
	}


	if ((bSamePos && !bExtruding) || (bSamePos && (m_uiZ == m_uiExtrEnd[1]))) //SamePos doesn't inculde Z so we don't get inf/zero slopes on hops.
	{
		return;
	}

	if (uiE>m_iEMax)
	{
		m_iEMax = uiE;
	}
	else if (!m_bExtruding && !bExtruding)
	{
		// Just update segment end if we are mid non-extrusion (travel)
		// We don't need to track co-linearity if not extruding.
		bColinear = true;
	}
	std::vector<float> fExtrEnd = {
		static_cast<float>(m_uiExtrEnd[0])/static_cast<float>(m_iStepsPerMM[0]*1000),
		static_cast<float>(m_uiExtrEnd[1])/static_cast<float>(m_iStepsPerMM[2]*1000),
		static_cast<float>(m_uiExtrEnd[2])/static_cast<float>(m_iStepsPerMM[1]*1000),
		};

	std::array<float,3> vfPos =
	{{
		static_cast<float>(m_uiX)/static_cast<float>(m_iStepsPerMM[0]*1000),
		static_cast<float>(m_uiZ)/static_cast<float>(m_iStepsPerMM[2]*1000),
		static_cast<float>(m_uiY)/static_cast<float>(m_iStepsPerMM[1]*1000)
	}};
	if (bExtruding ^ m_bExtruding)
	{
		// Extruding condition has changed. Start a new segment.
		if (bExtruding) // Just started extruding. Update the various pointers.
		{

			//m_ivTStart.push_back(m_fvTri.size()/3);
			// if (fZ>m_fCurZ)
			{
				//printf("New Z layer: %.02f\n", fZ);
				//m_fLastZ = m_fCurZ;
				//m_fCurZ = fZ;
			}
			//printf("New extrusion %u at index %u\n",m_ivStart.size(),m_ivStart.back());
			// Add a temporary normal vertex
			std::vector<float> fCross = {0,0,0}, fA = {0,0,0}, fB = {0,-1,0};
			std::transform(vfPos.begin(), vfPos.end(), fExtrEnd.data(), fA.data(), std::minus<float>());
			CrossProduct(fA,fB,{fCross.data(),3});
			Normalize({fCross.data(),3});
			m_vPath.clear();
			std::lock_guard<std::mutex> lock(m_lock); // Lock out GL while updating vectors

			m_uiExtrStart = m_uiExtrEnd;
			m_ivStart.push_back(m_fvDraw.size()/3); // Index of what we're about to add...
			m_fvDraw.insert(m_fvDraw.end(),fExtrEnd.begin(), fExtrEnd.end());
			m_fvNorms.insert(m_fvNorms.end(), fCross.begin(), fCross.end());

		}
		m_vPath.push_back({m_uiExtrEnd[0], m_uiExtrEnd[2], m_uiExtrEnd[3]});
		if (!bExtruding)
		{
			{
				std::lock_guard<std::mutex> lock(m_lock);
				m_ivCount.push_back((m_fvDraw.size()/3) - m_ivStart.back());
			}
			//printf("Ended extrusion %u (%u vertices)\n", m_ivCount.size(), m_ivCount.back());
			if (m_b3DExt)
			{
				AddSegment();
			}
		}
		m_bExtruding = bExtruding;
	}
	else if (!bColinear)
	{
		// First, update the previous normal with the new vertex info.
		std::vector<float> fCross = {0,0,0}, fA = {0,0,0} ,fB = {0,-0.002,0};
		auto itPrev = m_fvNorms.end()-3;
		std::transform(itPrev, itPrev+3, vfPos.data(), fA.data(), std::minus<float>()); // Length from p->curr
		CrossProduct(fA,fB,{fCross.data(),3});
		Normalize({fCross.data(),3});
		auto itCross = fCross.begin();
		for (int i=0; i<3; i++)
		{
			*itPrev += *itCross;
			*itPrev/=2.f;
			itPrev++;
			itCross++;
		}
		// And then append the new temporary end one.
		std::transform(vfPos.begin(), vfPos.end(), fExtrEnd.data(), fA.data(), std::minus<float>());
		CrossProduct(fA,fB,{fCross.data(),3});
		Normalize({fCross.data(),3});
				// New segment, push it onto the vertex list and update the segment count
		//printf("New segment: %d\n",m_vCoords.size());
		m_vPath.push_back({m_uiExtrEnd[0], m_uiExtrEnd[2], m_uiExtrEnd[3]});
		{
			std::lock_guard<std::mutex> lock(m_lock); // Lock out GL while updating vectors
			m_fvDraw.insert(m_fvDraw.end(),fExtrEnd.begin(), fExtrEnd.end());
			m_fvNorms.insert(m_fvNorms.end(), fCross.begin(), fCross.end());
			m_uiExtrStart = m_uiExtrEnd;
		}


	}
	// Update the end we are tracking.
	m_fLastERate = fERate;
	m_uiExtrEnd[0] = m_uiX;
	m_uiExtrEnd[2] = m_uiY;
	m_uiExtrEnd[1] = m_uiZ;
	m_uiExtrEnd[3] = m_uiE;
	{
		std::lock_guard<std::mutex> lock (m_lock);
		m_fExtrEnd = {{fExtrEnd[0],fExtrEnd[1],fExtrEnd[2]}};
	}

}

// Does the computational geometry and pushes the extrusion:
// NOTE: All math results/positions in mm for sanity; the draw function does a scale by 1/1000.
static constexpr float fLayerZRad = 0.0001f; //0.5*layer height. TODO (vintagepc): Sort this out based on guessed z height.
void GLPrint::AddSegment()
{
	static constexpr float FILAMENT_AREA_COEFF = (.00175f*.00175f)/4.f; // No pi because it factors out later anyway.
	static constexpr float fNarrow[3] = {0,1,1}, fWide[3] = {1,0,0} ;

	//std::cout << "Adding segment from: " << extPrev[0] << " " << fZ << " " << fY << " to " << m_fExtrEnd[0] << " " << m_fExtrEnd[1] << " " << m_fExtrEnd[2] << '\n';
	std::vector<float> fCross = {0,0,0}, fA = {0,0,0} ,fB = {0,-2,0};

	for(auto it = m_vPath.begin(); it!=m_vPath.end()-1; it++)
	{
		auto pt = *it, ptNext = *std::next(it);
		float fZ = (static_cast<float>(m_uiExtrEnd[1])/static_cast<float>(m_iStepsPerMM[2]*1000)) - fLayerZRad;
		auto iX = std::get<0>(pt), iY = std::get<1>(pt), iE = std::get<2>(pt);
		auto iXN = std::get<0>(ptNext), iYN = std::get<1>(ptNext), iEN = std::get<2>(ptNext);
		int32_t idE = gsl::narrow<int32_t>(iEN) - iE; // E linear distance.
		if (idE <0)
		{
			continue;
		}
		int32_t dX = gsl::narrow<int32_t>(iXN) - iX;
		int32_t dY = gsl::narrow<int32_t>(iYN) - iY;
		fA[0] = dX;
		fA[2] = dY;
		fA[0]/= static_cast<float>(m_iStepsPerMM[0]*1000);
		fA[2]/= static_cast<float>(m_iStepsPerMM[1]*1000);
		auto fX = static_cast<float>(iX)/static_cast<float>(m_iStepsPerMM[0]*1000);
		auto fXN = static_cast<float>(iXN)/static_cast<float>(m_iStepsPerMM[0]*1000);
		auto fY = static_cast<float>(iY)/static_cast<float>(m_iStepsPerMM[1]*1000);
		auto fYN = static_cast<float>(iYN)/static_cast<float>(m_iStepsPerMM[1]*1000);
		// Approximate the resulting extrusion width with an ellipse.
		float fdXY = std::sqrt((fA[0]*fA[0])+(fA[2]*fA[2])); // Length of extrusion on print surface.
		float fExtrVol =  FILAMENT_AREA_COEFF *(static_cast<float>(idE)/static_cast<float>(m_iStepsPerMM[3]*1000));
		float fExtRad = (fExtrVol/(fLayerZRad*fdXY)); // Should give us the XY radius of the extrusion ellipse.
		//std::cout << "Seg: " << fX << " \t" << fY << "\t E:" << idE << "\t R:" << fExtRad << '\n';

		Color3fv colW;
		colorLerp(fNarrow, fWide, fExtRad/0.002, colW);
		CrossProduct(fB,fA,{fCross.data(),3});
		Normalize({fCross.data(),3});
		auto fCrossRev = fCross;
		fCrossRev[0]= -fCross[0];
		fCrossRev[2]= -fCross[2];
		{
			auto iTStart = m_fvTri.size()/3;
			std::lock_guard<std::mutex> lock(m_lock);
			m_fvTri.reserve(m_fvTri.size()+30);
			m_fvTriNorm.reserve(m_fvTri.size());

			m_fvTri.insert(m_fvTri.end(), {fXN+(fCross[0]*fExtRad), fZ, fYN+(fCross[2]*fExtRad)});
			m_fvTriNorm.insert(m_fvTriNorm.end(), fCross.begin(), fCross.end());

			m_fvTri.insert(m_fvTri.end(), {fX+(fCross[0]*fExtRad), fZ, fY+(fCross[2]*fExtRad)});
			m_fvTriNorm.insert(m_fvTriNorm.end(), fCross.begin(), fCross.end());

			m_fvTri.insert(m_fvTri.end(), {fXN, fZ+fLayerZRad, fYN});
			m_fvTriNorm.insert(m_fvTriNorm.end(), {0,1,0});

			m_fvTri.insert(m_fvTri.end(), {fX, fZ+fLayerZRad, fY});
			m_fvTriNorm.insert(m_fvTriNorm.end(), {0,1,0});

			m_fvTri.insert(m_fvTri.end(), {fXN-(fCross[0]*fExtRad), fZ, fYN-(fCross[2]*fExtRad)});
			m_fvTriNorm.insert(m_fvTriNorm.end(), fCrossRev.begin(), fCrossRev.end());

			m_fvTri.insert(m_fvTri.end(), {fX-(fCross[0]*fExtRad), fZ, fY-(fCross[2]*fExtRad)});
			m_fvTriNorm.insert(m_fvTriNorm.end(), fCrossRev.begin(), fCrossRev.end());

			m_fvTri.insert(m_fvTri.end(), {fXN, fZ-fLayerZRad, fYN});
			m_fvTriNorm.insert(m_fvTriNorm.end(), {0,-1,0});

			m_fvTri.insert(m_fvTri.end(), {fX, fZ-fLayerZRad, fY});
			m_fvTriNorm.insert(m_fvTriNorm.end(), {0,-1,0});

			m_fvTri.insert(m_fvTri.end(), {fXN+(fCross[0]*fExtRad), fZ, fYN+(fCross[2]*fExtRad)});
			m_fvTriNorm.insert(m_fvTriNorm.end(), fCross.begin(), fCross.end());

			m_fvTri.insert(m_fvTri.end(), {fX+(fCross[0]*fExtRad), fZ, fY+(fCross[2]*fExtRad)});
			m_fvTriNorm.insert(m_fvTriNorm.end(), fCross.begin(), fCross.end());

			if (m_bColExt)
			{
				m_vfTriColor.reserve(m_fvTri.size());
				for (int i=0; i<10; i++)
				{
					m_vfTriColor.insert(m_vfTriColor.end(), {colW[0], colW[1], colW[2]});
				}
			}

			m_ivTStart.push_back(iTStart);
			m_ivTCount.push_back((m_fvTri.size()/3) - iTStart);
		}
	}
}

void GLPrint::Draw()
{
	std::vector<float> fColor = {m_fColR,m_fColG,m_fColB,1};
	//std::vector<float> fG[4] = {0,0.5,0,1};
	std::vector<float> fY = {1,1,0,1};
	std::vector<float> fK = {0,0,0,1};
	std::vector<float> fSpec = {1,1,1,1};
	glLineWidth(1.0);

	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,fSpec.data());
	glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,64);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fColor.data());
		{
			if (m_b3DExt)
			{
				std::lock_guard<std::mutex> lock(m_lock);
				glVertexPointer(3, GL_FLOAT, 3*sizeof(float), m_fvTri.data());
				glNormalPointer(GL_FLOAT, 3*sizeof(float), m_fvTriNorm.data());
				if (m_bColExt)
				{
					glEnable(GL_COLOR_MATERIAL);
					glEnableClientState(GL_COLOR_ARRAY);
					glColorPointer(3, GL_FLOAT, 3*sizeof(float), m_vfTriColor.data());
				}
				glMultiDrawArrays(GL_TRIANGLE_STRIP,m_ivTStart.data(),m_ivTCount.data(), m_ivTCount.size());
				if (m_bColExt)
				{
					glDisable(GL_COLOR_MATERIAL);
					glDisableClientState(GL_COLOR_ARRAY);
				}
			}
			else
			{
				std::lock_guard<std::mutex> lock(m_lock);
				glVertexPointer(3, GL_FLOAT, 3*sizeof(float), m_fvDraw.data());
				glNormalPointer(GL_FLOAT, 3*sizeof(float), m_fvNorms.data());
				glMultiDrawArrays(GL_LINE_STRIP,m_ivStart.data(),m_ivCount.data(), m_ivCount.size());
			}

			glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fSpec.data());
			if (m_ivCount.size()>0) // the "In progress" segments
			{
				glDrawArrays(GL_LINE_STRIP,m_ivStart.back(),((m_fvDraw.size()/3)-m_ivStart.back())-1);
			}
			if (m_bExtruding && m_fvDraw.size() >0)
			{
				glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fY.data());
				glBegin(GL_LINES);
					glVertex3fv(&*(m_fvDraw.end()-3));
					glVertex3fv(m_fExtrEnd.data());
				glEnd();
			}
		}
		// Uncomment for vertex debugging.
		 //glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,fG);
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
