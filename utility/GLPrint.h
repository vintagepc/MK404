/*
	GLPrint.h - Object responsible for print visualization

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

#include "gsl-lite.hpp"
#include <array>   // for array
#include <atomic>
#include <cmath>   // for sqrt
#include <mutex>
#include <vector>  // for vector

class GLPrint
{
	public:

	// Creates a new GLPrint.
	GLPrint(float fR, float fG, float fB);

	// Clears the current print from the bed. You probably shouldn't call this when mid print.
	void Clear();

	// Draws the print within the current GL matrix context.
	void Draw();

	// Function to receive new coordinate updates from your simulated printer's stepper drivers.
	void NewCoord(float fX, float fY, float fZ, float fE);

	private:

		static inline void CrossProduct(const std::vector<float>&fA, const std::vector<float>&fB, gsl::span<float>fOut)
		{
			fOut[0] = (fA[1]*fB[2]) - (fA[2]*fB[1]);
			fOut[1] = (fA[2]*fB[0]) - (fA[0]*fB[2]);
			fOut[2] = (fA[0]*fB[1]) - (fA[1]*fB[0]);
		};

		static inline void Normalize(gsl::span<float>fA)
		{
			float fNorm = std::sqrt((fA[0]*fA[0]) + (fA[1]*fA[1]) + (fA[2]*fA[2]));
			fA[0]/=fNorm;
			fA[1]/=fNorm;
			fA[2]/=fNorm;
		}

		//void FindNearest(const float fVec[3]);

		void AddSegment();//(const std::array<float, 4> &fvEnd, gsl::span<float> &fvPrev);

		std::array<int,4> m_iExtrEnd = {{0,0,0,0}}, m_iExtrStart = {{0,0,0,0}};
		std::array<float,4> m_fExtrEnd = {{0,0,0,0}}, m_fExtrStart = {{0,0,0,0}}, m_fExtrPrev = m_fExtrEnd;

		std::vector<int> m_ivStart, m_ivTStart;
		std::vector<int> m_ivCount, m_ivTCount;
		std::vector<float> m_fvDraw, m_fvNorms;
		std::vector<float> m_fvTri, m_fvTriNorm;
		// Layer vertex tracking.
		std::vector<float*> m_vpfLayer1, m_vpfLayer2;
		// std::vector<float*> *m_pCurLayer = &m_vpfLayer1;   // not used
		// std::vector<float*> *m_pPrevLayer = &m_vpfLayer2;  // not used
		float m_fCurZ = -1;
		// float m_fLastZ = -1;                          // not used
		float m_fEMax = 0;
		const float m_fColR, m_fColG, m_fColB;
		std::atomic_bool m_bExtruding = {false};
		std::vector<std::tuple<float,float,float>> m_fvPath;

		std::mutex m_lock;
};
