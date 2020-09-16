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

#include "Prusa_MK2MMU_13.h"
#include "MMU1.h"                   // for MMU1, MMU1::IRQ::MUX0, MMU1::IRQ:...
#include "PinNames.h"               // for Pin::E_MUX0_PIN, Pin::E_MUX1_PIN
#include "printers/Prusa_MK2_13.h"  // for Prusa_MK2_13

void Prusa_MK2MMU_13::SetupHardware()
{
	Prusa_MK2_13::SetupHardware();

	AddHardware(m_mmu);
	TryConnect(E_MUX0_PIN, m_mmu, MMU1::MUX0);
	TryConnect(E_MUX1_PIN, m_mmu, MMU1::MUX1);
}
