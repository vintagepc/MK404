/*
	GLPrint.h - Object responsible for print visualization

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

#include <vector>
#include <tuple>
#include <math.h>

class GLPrint
{
	public:

	// Creates a new GLPrint.
	GLPrint();

	// Clears the current print from the bed. You probably shouldn't call this when mid print.
	void Clear();

	// Draws the print within the current GL matrix context.
	void Draw();

	// Function to receive new coordinate updates from your simulated printer's stepper drivers. 
	void NewCoord(float fX, float fY, float fZ, float fE);

	private:

		static inline void CrossProduct(const float fA[3], const float fB[3], float fOut[3]) 
		{	
			fOut[0] = (fA[1]*fB[2]) - (fA[2]*fB[1]);
			fOut[1] = (fA[2]*fB[0]) - (fA[0]*fB[2]);
			fOut[2] = (fA[0]*fB[1]) - (fA[1]*fB[0]);
		};

		static inline void Normalize(float fA[3])
		{
			float fNorm = sqrt((fA[0]*fA[0]) + (fA[1]*fA[1]) + (fA[2]*fA[2]));
			fA[0]/=fNorm;
			fA[1]/=fNorm;
			fA[2]/=fNorm;
		}

		//void FindNearest(const float fVec[3]);


		std::array<int,4> m_iExtrEnd, m_iExtrStart;
		std::array<float,4> m_fExtrEnd, m_fExtrStart;
		std::vector<int> m_ivStart, m_ivTStart;
		std::vector<int> m_ivCount, m_ivTCount;
		std::vector<float> m_fvDraw, m_fvNorms;
		std::vector<float> m_fvTri;
		// Layer vertex tracking.
		std::vector<float*> m_vpfLayer1, m_vpfLayer2, *m_pCurLayer = &m_vpfLayer1, *m_pPrevLayer = &m_vpfLayer2;
		float m_fCurZ = -1, m_fLastZ = -1;
		float m_fEMax = 0;
		bool m_bExtruding = false;
};