/*
	Prusa_MK25S_13.cpp - Printer definition for the Prusa MK2.5S (mR1.3)
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

#include "Prusa_MK25S_13.h"
#include "A4982.h"
#include "IRSensor.h"  // for IRSensor
#include "LED.h"       // for LED, LED::IRQ::LED_IN, PAT9125::IRQ::LED_OUT
#include "PinNames.h"  // for Pin::IR_SENSOR_PIN, Pin::VOLT_IR_PIN

void Prusa_MK25S_13::SetupHardware()
{
	Prusa_MK25_13::SetupHardware();
	Z.GetConfig().iMaxMM = 215;
	Z.ReparseConfig();
}

void Prusa_MK25S_13::SetupFilamentSensor()
{
	AddHardware(m_IR, GetPinNumber(VOLT_IR_PIN));
	TryConnect(&m_IR,IRSensor::DIGITAL_OUT, IR_SENSOR_PIN);
	TryConnect(IR_SENSOR_PIN, &lIR, LED::LED_IN);
	m_IR.Set(IRSensor::IR_v4_FILAMENT_PRESENT);
}
