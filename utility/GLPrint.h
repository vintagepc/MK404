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

#include "PrintVisualType.h"
#include <array>   // for array
#include <atomic>
#include <cstdint>
#include <mutex>
#include <tuple>
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

	// Functions to receive new coordinate updates from your simulated printer's stepper drivers.

	// Swap these two to enable simulated nonlinearity on X.

	inline void OnXStep(const uint32_t &value) { m_uiX = m_bNLX ? GetAdjustedStep(value) : value;}

	inline void OnYStep(const uint32_t &value) { m_uiY = m_bNLY ? GetAdjustedStep(value) : value;}

	inline void OnZStep(const uint32_t &value) { m_uiZ = m_bNLZ ? GetAdjustedStep(value) : value;}

	void OnEStep(const uint32_t &value, const uint32_t &deltaT);

	inline void SetStepsPerMM(int16_t iX, int16_t iY, int16_t iZ, int16_t iE)
	{
		m_iStepsPerMM = {iX, iY, iZ, iE};
	}

	// Enable/disable NL behaviour. (ab)use assignment returns for return val.
	inline bool ToggleNLX() { return m_bNLX = !m_bNLX;}
	inline bool ToggleNLY() { return m_bNLY = !m_bNLY;}
	inline bool ToggleNLZ() { return m_bNLZ = !m_bNLZ;}
	inline bool ToggleNLE() { return m_bNLE = !m_bNLE;}

	private:

		void AddSegment();//(const std::array<float, 4> &fvEnd, gsl::span<float> &fvPrev);


		// This is a function to calculate simulated stepper non linearity
		static uint32_t GetAdjustedStep(uint32_t uiStep);

		std::atomic_uint32_t m_uiX {0}, m_uiY {0}, m_uiZ {0}, m_uiE {0};

		std::array<uint32_t,4> m_uiExtrEnd = {{0,0,0,0}}, m_uiExtrStart = {{0,0,0,0}};

		std::array<float, 3> m_fExtrEnd = {{0,0,0}};

		std::vector<int16_t> m_iStepsPerMM = {0,0,0};

		// pre-allocation size for the vectors - saves a lot of CPU cycles
		// when adding items into vectors (the vector will not get reallocated with every insertion)
		static constexpr size_t VectorPrealoc = 2000000; // 2M items
		static constexpr size_t VectorPrealoc3 = 3*VectorPrealoc; // 6M items

		std::vector<int> m_ivStart, m_ivTStart;
		std::vector<int> m_ivCount, m_ivTCount;
		std::vector<float> m_fvDraw, m_fvNorms;
		std::vector<float> m_fvTri, m_fvTriNorm, m_vfTriColor;
		// Layer vertex tracking.
		std::vector<float*> m_vpfLayer1, m_vpfLayer2;
		// std::vector<float*> *m_pCurLayer = &m_vpfLayer1;   // not used
		// std::vector<float*> *m_pPrevLayer = &m_vpfLayer2;  // not used
		float m_fPrevZ = -1, m_fCurZ = 0, m_fZHt = 1.001;
		// float m_fLastZ = -1;                          // not used
		uint64_t m_iEMax = 0;
		bool m_bFirst = true;
		float m_fLastERate = 0;
		const float m_fColR, m_fColG, m_fColB;
		std::atomic_bool m_bExtruding = {false}, m_bNLX {false}, m_bNLY {false}, m_bNLZ {false}, m_bNLE {false};
		// {X, Y, Z, E, dT}
		std::vector<std::tuple<uint32_t,uint32_t,uint32_t,uint32_t>> m_vPath;

		std::mutex m_lock;

		unsigned int m_iVisType = PrintVisualType::LINE, m_iBaseMode = PrintVisualType::QUAD;

		bool m_bHRE = false, m_bColExt = false;
};
