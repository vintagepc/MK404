/*
	IRQHelper.h - This .h file generates the enumeration and name pairs.
    To use it, #define IRQPAIRS as a list of _IRQ(enum,name) commands.
    Then include this header right below. It will expand into the enum{}
    and the const char* [] that SimAVR requires.

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
#ifdef _IRQ
#undef _IRQ
#endif

#define _IRQ(x,y) x,
enum IRQ {
    IRQPAIRS
    COUNT
};
#undef _IRQ

#define _IRQ(x,y) y,
const char *_IRQNAMES[IRQ::COUNT] = {
    IRQPAIRS
    };
#undef _IRQ

#undef IRQPAIRS // Clear the IRQ pairs for subsequent classes.
// Leave this here for IWYU:
#define _IRQ(x,y)
