/*
	PinSpec_Helper.h - Pin specification helper class. Include this when defining
	a PinSpec.

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


// Shortcuts to make it easy to copy-paste tables rather than
// having to alter every entry:
#define PA 'A'
#define PB 'B'
#define PC 'C'
#define PD 'D'
#define PE 'E'
#define PF 'F'
#define PG 'G'
#define PH 'H'
#define PJ 'J'
#define PK 'K'
#define PL 'L'

// Timers defined with 4 MSBs as the port, 4 LSB as the PWM index
#define NOT_ON_TIMER 0xFF
#define TIMER0A 0x00
#define TIMER0B 0x01
#define TIMER1A 0x10
#define TIMER1B 0x11
#define TIMER2A 0x20
#define TIMER2B 0x21
#define TIMER3A 0x30
#define TIMER3B 0x31
#define TIMER3C 0x32
#define TIMER4A 0x40
#define TIMER4B 0x41
#define TIMER4C 0x42
#define TIMER4D 0x43
#define TIMER5A 0x50
#define TIMER5B 0x51
#define TIMER5C 0x52

#define _BV(w)w
