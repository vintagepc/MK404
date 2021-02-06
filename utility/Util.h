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

#include <cstdint>
#include <cstring>
#include <cxxabi.h>
#include <string>

struct _hexColor_t{
	//NOLINTNEXTLINE - we want the implicit conversion...
	_hexColor_t(const uint32_t &val){ memcpy(this, &val, 4);} // Helper constructor
	_hexColor_t(const uint32_t &val, const float &fScale)
	{
		memcpy(this, &val, 4);
		alpha = static_cast<float>(alpha)*fScale;
		red = 	static_cast<float>(red)*fScale;
		green = static_cast<float>(green)*fScale;
		blue = 	static_cast<float>(blue)*fScale;
	} // Helper constructor
	uint8_t alpha{0};
	uint8_t blue {0};
	uint8_t green {0};
	uint8_t red {0};
};

using hexColor_t = _hexColor_t;

// Helper to demangle C++ typenames
inline const std::string CXXDemangle(const char* strMangled)
{
	int status = 0;
	char * demangled = abi::__cxa_demangle(strMangled,nullptr,nullptr,&status);
    std::string strDemang {demangled};
    free(demangled); // NOLINT, no alternative.
	return strDemang;
}
