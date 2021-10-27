/*
	PinSpec_32u4.h - Pin translation layer for pins_Einsy.h (arduino pin numbers, ##) to the ATMEL port designator (Px#).
	You will need to add a new layer for other boards if yours does not match this, but it should work for all
	Atmel 32u4-derivative boards.

	This has been unceremoniously lifed from the pins_arduino.h file in the Prusa board definition for the MMU.

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

#include "PinSpec.h"
#include "PinSpec_Helper.h"

class PinSpec_32u4 : public PinSpec
{
	public:
		// Creates a new 32u4 pinspec.
		PinSpec_32u4():PinSpec(DPin2Port,DPin2Mask,DPin2Timer,"atmega32u4"){};

	private:
		const unsigned char DPin2Timer[32] = {
			// TIMERS
			// -------------------------------------------
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			TIMER0B,        /* 3 */
			NOT_ON_TIMER,
			TIMER3A,        /* 5 */
			TIMER4D,        /* 6 */
			NOT_ON_TIMER,

			NOT_ON_TIMER,
			TIMER1A,        /* 9 */
			TIMER1B,        /* 10 */
			TIMER0A,        /* 11 */

			NOT_ON_TIMER,
			TIMER4A,        /* 13 */

			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,

			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
			NOT_ON_TIMER,
		};


		const unsigned char DPin2Mask[32] = {
			// PIN IN PORT
			// -------------------------------------------
			_BV(2), // D0 - PD2
			_BV(3), // D1 - PD3
			_BV(1), // D2 - PD1
			_BV(0), // D3 - PD0
			_BV(4), // D4 - PD4
			_BV(6), // D5 - PC6
			_BV(7), // D6 - PD7
			_BV(6), // D7 - PE6

			_BV(4), // D8 - PB4
			_BV(5), // D9 - PB5
			_BV(6), // D10 - PB6
			_BV(7), // D11 - PB7
			_BV(6), // D12 - PD6
			_BV(7), // D13 - PC7

			_BV(3), // D14 - MISO - PB3
			_BV(1), // D15 - SCK - PB1
			_BV(2), // D16 - MOSI - PB2
			_BV(0), // D17 - SS - PB0

			_BV(7), // D18 - A0 - PF7
			_BV(6), // D19 - A1 - PF6
			_BV(5), // D20 - A2 - PF5
			_BV(4), // D21 - A3 - PF4
			_BV(1), // D22 - A4 - PF1
			_BV(0), // D23 - A5 - PF0

			_BV(4), // D24 / D4 - A6 - PD4
			_BV(7), // D25 / D6 - A7 - PD7
			_BV(4), // D26 / D8 - A8 - PB4
			_BV(5), // D27 / D9 - A9 - PB5
			_BV(6), // D28 / D10 - A10 - PB6
			_BV(6), // D29 / D12 - A11 - PD6
			_BV(5), // D30 / TX Led - PD5
			_BV(2), // D31, HWB
		};

		const unsigned char DPin2Port[32] = {
			// PORTLIST
			// -------------------------------------------
			PD, // D0 - PD2
			PD, // D1 - PD3
			PD, // D2 - PD1
			PD, // D3 - PD0
			PD, // D4 - PD4
			PC, // D5 - PC6
			PD, // D6 - PD7
			PE, // D7 - PE6

			PB, // D8 - PB4
			PB, // D9 - PB5
			PB, // D10 - PB6
			PB, // D11 - PB7
			PD, // D12 - PD6
			PC, // D13 - PC7

			PB, // D14 - MISO - PB3
			PB, // D15 - SCK - PB1
			PB, // D16 - MOSI - PB2
			PB, // D17 - SS - PB0

			PF, // D18 - A0 - PF7
			PF, // D19 - A1 - PF6
			PF, // D20 - A2 - PF5
			PF, // D21 - A3 - PF4
			PF, // D22 - A4 - PF1
			PF, // D23 - A5 - PF0

			PD, // D24 / PD5
			PD, // D25 / D6 - A7 - PD7
			PB, // D26 / D8 - A8 - PB4
			PB, // D27 / D9 - A9 - PB5
			PB, // D28 / D10 - A10 - PB6
			PD, // D29 / D12 - A11 - PD6
			PD, // D30 / TX Led - PD5

			PE, // D31, HWB
		};

};
