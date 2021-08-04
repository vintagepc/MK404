/*
	GLMotor.h - Generic motor drawable

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

#include <atomic>
#include <cstdint>            // for uint8_t, uint32_t, int32_t, uint16_t

class IPCPrinter;

class GLMotor
{
	friend IPCPrinter;
	public:
		// Default constructor.
		explicit GLMotor(char cAxis = ' '):m_cAxis(cAxis){};

		virtual ~GLMotor() = default;


		// Draws a simple visual representation of the motor position.
		void Draw();

		// Sets whether to draw positional ref or just a number.
		inline void SetSimple(bool bVal) { m_bIsSimple = bVal; }

	protected:

		inline void SetEnable(bool bVal) { m_bEnable = bVal; }
		inline void SetMaxPos(int32_t iVal)
		{
			m_iMaxPos = iVal;
			m_fEnd = StepToPos(iVal);
		}
		inline void SetCurrentPos(int32_t iVal)
		{
			m_iCurStep = iVal;
			m_fCurPos = StepToPos(iVal);
		}
		inline void SetStepsPerMM(uint32_t uiVal) { m_uiStepsPerMM = uiVal; }
		inline void SetAxis(char cVal) { m_cAxis = cVal; }

		inline float GetCurrentPos() { return m_fCurPos.load(); }

		int32_t m_iCurStep = 0;
		int32_t m_iMaxPos = 0;

		std::atomic_bool 	m_bEnable {true},
							m_bIsSimple{false},
							m_bStealthMode {false},
							m_bDrawStall {false},
							m_bConfigured {false};

		std::atomic<float> m_fCurPos = {0}, m_fEnd = {0}; // Tracks position in float for gl
		std::atomic_char m_cAxis {' '};
		// Position helpers
		virtual float StepToPos(int32_t step);
		virtual int32_t PosToStep(float step);

	private:


		uint32_t m_uiStepsPerMM = 0;



};
