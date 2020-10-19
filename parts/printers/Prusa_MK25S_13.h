/*
	Prusa_MK25S_13.h - Printer definition for the Prusa MK2.5S (mR 1.3)
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

#pragma once

#include "IRSensor.h"
#include "Prusa_MK25_13.h"     // for EinsyRambo


class Prusa_MK25S_13 : public Prusa_MK25_13
{
	protected:
		void SetupHardware() override;

		void SetupFilamentSensor() override;

		IRSensor m_IR{};

};
