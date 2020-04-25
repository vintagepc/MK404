/*

    Macros.h - convenience macros for working with SimAVR.

*/

#ifndef __MACROS_H__
#define __MACROS_H__

#include "include/PinHelper_2560.h"

// Shorthand to get IRQ for a digital IO port.
#define IOIRQ(avr,port,number) avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ(port),number)

#define TIMERPWMNAME(n) TIMER_IRQ_OUT_PWM0 + (n)

// Timer PWM shorthand function
#define TIMERIRQ(avr,timer,number)	avr_io_getirq(avr,AVR_IOCTL_TIMER_GETIRQ(timer),TIMERPWMNAME(number))

// Looks up a digital IRQ based on the arduino convenience pin number.
#define DIRQLU(avr,n) IOIRQ(avr,PORT(n),PIN(n))

// Looks up a PWM IRQ based on the arduino convenience pin number.
#define DPWMLU(avr,n) TIMERIRQ(avr, TIMER_CHAR(n), TIMER_IDX(n)) 

#endif