/*
	GLHelper.h - Lightweight helper for testing the GL drawing of parts.
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

#include "IScriptable.h"
#include "Scriptable.h"
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

class GLHelper: public Scriptable
{
	public:
		explicit GLHelper(const std::string &strName = "GLHelper");

		inline bool IsTakingSnapshot() { return m_iState >= St_Queued; }

		// Function for running the GL stuff inside the GL context.
		void OnDraw();

	protected:

		bool WritePNG(int width, int height, bool bRegion);

		// Useful for debugging as it's a very simple format.
		// bool WritePPM()

		LineStatus ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs) override;

		enum Actions
		{
			ActCheckPixel,
			ActTakeSnapshot,
			ActTakeSnapshotArea
		};
		enum ActState
		{
			St_Idle,
			St_Done, // DO NOT REORDER
			St_Queued, // we check against >=QUEUED to determine if a snapshot is in progress.
			St_Queued2, // Used to delay 2 frames before taking a snap.
			St_Busy
		};
		std::string m_strFile;
		std::atomic_int m_iAct {-1};
		std::atomic_uint32_t m_x{0}, m_y{0}, m_h{0}, m_w{0}, m_color{0};
		std::atomic_int m_iState {St_Idle};
		std::vector<uint8_t> m_vBuffer{};
		bool m_bVFlip = true;
};
