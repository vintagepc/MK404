/*
	Prusa_MK1_13.cpp - Printer definition for the Prusa MK1 (mR1.3)
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

#include "Prusa_MK1_13.h"

#include "A4982.h"                          // for A4982
#include "PinNames.h"
#include "printers/Prusa_MK2_13.h"          // for Prusa_MK2_13

void Prusa_MK1_13::SetupHardware()
{
	Prusa_MK2_13::SetupHardware();
	X.GetConfig().iMaxMM = 214;
	X.ReparseConfig();
	Y.GetConfig().iMaxMM = 198;
	Y.ReparseConfig();
	TryConnect(Z,A4982::MIN_OUT,	Z_MIN_PIN);
	Z.GetConfig().iMaxMM = 201;
	Z.GetConfig().uiStepsPerMM = 4000;
	Z.ReparseConfig();
	E.GetConfig().uiStepsPerMM = 174;
	E.ReparseConfig();
}

inline void Prusa_MK1_13::SetupPINDA()
{

}
