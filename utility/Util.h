/*

    Util.h - convenience helpers for working with SimAVR.

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

#include <cstring>

typedef struct hexColor_t{
	hexColor_t(const uint32_t &val){ memcpy(this, &val, 4);} // Helper constructor
	hexColor_t(const uint32_t &val, const float &fScale)
	{
		memcpy(this, &val, 4);
		alpha = (float)alpha/fScale;
		red = (float)red/fScale;
		green = (float)green/fScale;
		blue = (float)blue/fScale;
	} // Helper constructor
	uint8_t alpha;
	uint8_t blue;
	uint8_t green;
	uint8_t red;
} hexColor_t;
