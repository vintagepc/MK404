/*
	PinSpec.h - Pin specification base class. Derive from this to make a new layer for a different ATMEL chip type.

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

#pragma once

#include <string>

class PinSpec {
	public:
		PinSpec(const unsigned char *pin2port, const unsigned char *pin2Mask, const unsigned char *pin2Timer,std::string strMCU):
			m_pDPin2Port(pin2port),m_pDPin2Mask(pin2Mask),m_pDPin2Timer(pin2Timer),m_strMCU(strMCU)
			{
				printf("Creating pinspec for %s\n",strMCU.c_str());
			};

		// Returns a char representation of the port, e.g. 'A'
		inline unsigned char PORT(unsigned int n) const {return m_pDPin2Port[n];}

		// Returns the port index/bitshift for the pin.
		inline unsigned char PIN(unsigned int n) const {return m_pDPin2Mask[n];}

		// Get Char rep of the timer port.
		inline unsigned char TIMER_CHAR(unsigned int n) const { return '0' + (m_pDPin2Timer[n]>>4); }

		// Get index of the timer
		inline unsigned char TIMER_IDX(unsigned int n) const { return m_pDPin2Timer[n]&0xF; }

		// Returns the MCU this spec is for. Used to designate the
		// CPU in a board using this pinspec.
		inline std::string GetMCUName() const { return m_strMCU; }

	protected:
		// Set these in your derived class constructor args to pointer
		// tables, such that array[i] is the value for Pin #i.
		const unsigned char* m_pDPin2Port = nullptr;
		const unsigned char* m_pDPin2Mask = nullptr;
		const unsigned char* m_pDPin2Timer = nullptr;

		std::string m_strMCU;
};
