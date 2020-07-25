/*
	Prusa_MK3.h - Printer definition for the Prusa MK3 (Laser sensor)
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

#pragma once

#include <stdio.h>
#include "sim_irq.h"
#include "Prusa_MK3S.h"     // for Prusa_MK3S
#include "PAT9125.h"

class MK3SGL;

class Prusa_MK3: public Prusa_MK3S
{
	protected:
		void SetupIR() override
		{
			avr_raise_irq(GetDIRQ(IR_SENSOR_PIN),1);
			printf("MK3 - adding laser sensor\n");
			AddHardware(LaserSensor, GetDIRQ(SWI2C_SCL), GetDIRQ(SWI2C_SDA));
			lIR.ConnectFrom(LaserSensor.GetIRQ(PAT9125::LED_OUT),LED::LED_IN);

			LaserSensor.ConnectFrom(E.GetIRQ(TMC2130::POSITION_OUT), PAT9125::E_IN);
			LaserSensor.Set(PAT9125::FS_NO_FILAMENT); // No filament - but this just updates the LED.
		}; // Overridde to setup the PAT.

		inline virtual void ToggleFSensor() override { LaserSensor.Toggle(); };

		inline virtual void FSensorJam() { LaserSensor.ToggleJam();};

		PAT9125 LaserSensor;
};
