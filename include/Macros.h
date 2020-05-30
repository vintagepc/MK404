/*

    Macros.h - convenience macros for working with SimAVR.

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

#ifndef __MACROS_H__
#define __MACROS_H__

// There's nothing here. Avoid macros if possible, they may not be typesafe. 
// Prefer things like (static) inlines, constexprs, and whatnot. Sometimes you have no choice.
// If that's the case, add it here.


// GL error checking macro. This needs some rework and should ideally be actually used in the code...
#define GL_DEBUG 1
#if GL_DEBUG
    #define GL_CHK_ERR(_w) _w; { int e = glGetError(); if (e) printf("GL Error on %s :: %d: %d\n", __FILE__,  __LINE__ ,e); };
#else
    #define GL_CHK_ERR(_w) _w
#endif

#endif