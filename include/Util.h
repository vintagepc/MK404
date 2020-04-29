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

#ifndef __UTIL_H__
#define __UTIL_H__

#include "PinHelper_2560.h"
#include "avr_ioport.h"
#include "avr_timer.h"

// Gets the PWM id for a given timer number.
inline static unsigned int _TimerPWMID(uint8_t number) { return TIMER_IRQ_OUT_PWM0 + number; }

// Shorthand to get IRQ for a digital IO port.
inline static struct avr_irq_t* IOIRQ(struct avr_t* avr,uint8_t port,uint8_t number) { return avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ(port),number); }

// Timer PWM shorthand function
inline static struct avr_irq_t* TIMERIRQ(struct avr_t* avr,uint8_t timer,uint8_t number)	{return avr_io_getirq(avr,AVR_IOCTL_TIMER_GETIRQ(timer), _TimerPWMID(number)); }

// Looks up a digital IRQ based on the arduino convenience pin number.
inline static struct avr_irq_t* DIRQLU(struct avr_t* avr, unsigned int n){ return IOIRQ(avr,PORT(n),PIN(n)); }

// Looks up a PWM IRQ based on the arduino convenience pin number.
inline static struct avr_irq_t* DPWMLU(struct avr_t* avr, unsigned int n) { return TIMERIRQ(avr, TIMER_CHAR(n), TIMER_IDX(n)); } 

#endif