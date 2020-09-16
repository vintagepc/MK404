/*
	Prusa_MK25_13.cpp - Printer definition for the Prusa MK2.5 (mR1.3)
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

#include "Prusa_MK25_13.h"
#include "3rdParty/MK3/thermistortables.h"  // for temptable_1, OVERSAMPLENR
#include "A4982.h"                          // for A4982
#include "Fan.h"                            // for Fan
#include "LED.h"                            // for LED
#include "PAT9125.h"                        // for PAT9125, A4982::IRQ::POSI...
#include "PINDA.h"                          // for PINDA, PINDA::XYCalMap
#include "PinNames.h"                       // for Pin::SWI2C_SCL, Pin::SWI2...
#include "Thermistor.h"                     // for Thermistor
#include "printers/Prusa_MK2_13.h"          // for Prusa_MK2_13
#include <cstdint>                         // for int16_t

void Prusa_MK25_13::SetupHardware()
{
	Prusa_MK2_13::SetupHardware();
	TryConnect(fExtruder, Fan::TACH_OUT, X_MAX_PIN);

	AddHardware(tPinda, GetPinNumber(TEMP_PINDA_PIN));
	//NOLINTNEXTLINE - so we can keep using thermistortables.h as-is.
	tPinda.SetTable({(int16_t*)temptable_1, sizeof(temptable_1)/sizeof(int16_t)}, OVERSAMPLENR);

	AddHardware(lIR);
	SetupFilamentSensor();

	pinda.Reconfigure(23.f, 5.f, PINDA::XYCalMap::MK25);

}

void Prusa_MK25_13::SetupFilamentSensor()
{
	AddHardware(m_fSensor, GetDIRQ(SWI2C_SCL), GetDIRQ(SWI2C_SDA));
	lIR.ConnectFrom(m_fSensor.GetIRQ(PAT9125::LED_OUT),LED::LED_IN);
	m_fSensor.ConnectFrom(E.GetIRQ(A4982::POSITION_OUT), PAT9125::E_IN);
	m_fSensor.Set(PAT9125::FS_NO_FILAMENT); // No filament - but this just updates the LED.
}
