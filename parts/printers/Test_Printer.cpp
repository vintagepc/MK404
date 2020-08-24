/*
	Part_Test.cpp - Printer definition for the part test printer
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

#include "Test_Printer.h"
#include "PinNames.h"         // for Pin::IR_SENSOR_PIN, Pin::VOLT_IR_PIN
#include "RotaryEncoder.h"    // for RotaryEncoder, RotaryEncoder::::CCW_CLICK


void Test_Printer::SetupHardware()
{
	Test_Board::SetupHardware();
}

void Test_Printer::OnAVRCycle()
{
	// int key = m_key;                            // copy atomic to local
	// if (key)
	// {
	// 	m_key = 0;
	// }
}
