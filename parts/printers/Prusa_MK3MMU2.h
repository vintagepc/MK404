/*
	Prusa_MK3MMU2.h - Printer definition for the Prusa MK3 w/MMU2
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

#include "PAT9125.h"
#include "Prusa_MK3SMMU2.h"     // for Prusa_MK3SMMU2

class Prusa_MK3MMU2 : public Prusa_MK3SMMU2
{
	protected:
		void SetupIR() override;

		PAT9125 LaserSensor;
};
