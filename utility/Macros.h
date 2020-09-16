/*

    Macros.h - convenience macros for working with SimAVR.

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

// There's nothing here. Avoid macros if possible, they may not be typesafe.
// Prefer things like (static) inlines, constexprs, and whatnot. Sometimes you have no choice.
// If that's the case, add it here.

// Quick and dirty token-paste macro for turning external #defines into *unsigned* values for bitwise
// operations, e.g. FLAG1 | FLAG2. Clang-tidy complains about them.
#define US(x) _CVT2U(x,U)

#define _CVT2U(x,y) x##y
#define _CVT2(x,y) x##.##y

#define FL(x) _CVT2(x,f)
