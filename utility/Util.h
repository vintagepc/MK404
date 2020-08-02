/*

    Util.h - convenience helpers for working with SimAVR.

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

typedef union hexColor_t{
	hexColor_t(const uint32_t &val){hex = val;} // Helper constructor
	uint32_t hex;
	struct{
		uint8_t alpha;
		uint8_t blue;
		uint8_t green;
		uint8_t red;
	};
	uint8_t bytes[4];
} hexColor_t;
