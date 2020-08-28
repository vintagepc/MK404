/*
	PinSpec.h - Pin specification base class. Derive from this to make a new layer for a different ATMEL chip type.

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
#include <iostream>
#include <string>


class PinSpec {
	public:
		PinSpec(gsl::span<const unsigned char>pin2port, gsl::span<const unsigned char>pin2Mask, gsl::span<const unsigned char>pin2Timer,const std::string &strMCU):
			m_pDPin2Port(pin2port),m_pDPin2Mask(pin2Mask),m_pDPin2Timer(pin2Timer),m_strMCU(strMCU)
			{
				std::cout << "Creating pinspec for" << strMCU << '\n';
			};

		// Returns a char representation of the port, e.g. 'A'
		inline unsigned char PORT(unsigned int n) const {return m_pDPin2Port[n];}

		// Returns the port index/bitshift for the pin.
		inline unsigned char PIN(unsigned int n) const {return m_pDPin2Mask[n];}

		// Get Char rep of the timer port.
		inline unsigned char TIMER_CHAR(unsigned int n) const { return '0' + (m_pDPin2Timer[n]>>4u); }

		// Get index of the timer
		inline unsigned char TIMER_IDX(unsigned int n) const { return m_pDPin2Timer[n]&0xFu; }

		// Returns the MCU this spec is for. Used to designate the
		// CPU in a board using this pinspec.
		inline std::string GetMCUName() const { return m_strMCU; }

	protected:
		// Set these in your derived class constructor args to pointer
		// tables, such that array[i] is the value for Pin #i.
		const gsl::span<const unsigned char> m_pDPin2Port;
		const gsl::span<const unsigned char> m_pDPin2Mask;
		const gsl::span<const unsigned char> m_pDPin2Timer;

		std::string m_strMCU;
};
