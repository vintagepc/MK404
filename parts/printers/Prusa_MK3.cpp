/*
	Prusa_MK3.h - Printer definition for the Prusa MK3 (Laser sensor)
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

#include "Prusa_MK3.h"     // for Prusa_MK3
#include "LED.h"       // for LED
#include "PinNames.h"  // for Pin::IR_SENSOR_PIN, Pin::SWI2C_SCL, Pin::SWI2C...
#include "TMC2130.h"   // for TMC2130
#include "sim_irq.h"   // for avr_raise_irq
#include <iostream>    // for operator<<, cout, ostream

void Prusa_MK3::SetupIR()
{
	avr_raise_irq(GetDIRQ(IR_SENSOR_PIN),1);
	std::cout << "MK3 - adding laser sensor\n";
	AddHardware(LaserSensor, GetDIRQ(SWI2C_SCL), GetDIRQ(SWI2C_SDA));
	lIR.ConnectFrom(LaserSensor.GetIRQ(PAT9125::LED_OUT),LED::LED_IN);

	LaserSensor.ConnectFrom(E.GetIRQ(TMC2130::POSITION_OUT), PAT9125::E_IN);
	LaserSensor.Set(PAT9125::FS_FILAMENT_PRESENT);
}; // Overridde to setup the PAT.
