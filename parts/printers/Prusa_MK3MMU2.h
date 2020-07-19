/*
	Prusa_MK3MMU2.h - Printer definition for the Prusa MK3 w/MMU2
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

#include <stdint.h>        // for uint32_t
#include <utility>         // for pair
#include "GCodeSniffer.h"  // for GCodeSniffer
#include "MMU2.h"          // for MMU2
#include "Printer.h"       // for Printer::VisualType
#include "Prusa_MK3S.h"    // for Prusa_MK3S
#include "PAT9125.h"
#include "sim_irq.h"       // for avr_irq_t
class SerialPipe;


class Prusa_MK3MMU2 : public Prusa_MK3S
{

	public:
		Prusa_MK3MMU2():Prusa_MK3S(){};
		~Prusa_MK3MMU2();

		void Draw() override;
		void OnVisualTypeSet(VisualType type) override;

		bool GetHasMMU() override {return true;}

		std::pair<int,int> GetWindowSize() override;

		void OnKeyPress(unsigned char key, int x, int y) override;

	protected:
		void SetupHardware() override;

		void OnMMUFeed(avr_irq_t *irq, uint32_t value);// Helper for MMU IR sensor triggering.

		MMU2 m_MMU;
		GCodeSniffer m_sniffer = GCodeSniffer('T');
		SerialPipe *m_pipe = nullptr;
		
		void SetupIR() override
		{
			avr_raise_irq(GetDIRQ(IR_SENSOR_PIN),1);
			printf("MK3 - adding laser sensor\n");
			AddHardware(LaserSensor, GetDIRQ(SWI2C_SCL), GetDIRQ(SWI2C_SDA));
			lIR.ConnectFrom(LaserSensor.GetIRQ(PAT9125::LED_OUT),LED::LED_IN);

			LaserSensor.ConnectFrom(E.GetIRQ(TMC2130::POSITION_OUT), PAT9125::E_IN);
			LaserSensor.Set(false); // No filament - but this just updates the LED.
		}; // Overridde to setup the PAT.

		inline virtual void ToggleFSensor() override { LaserSensor.Toggle(); };

		PAT9125 LaserSensor;

	private:


};
