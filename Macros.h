/*

    Macros.h - convenience macros for working with SimAVR.

*/

#include "include/PinHelper_2560.h"

#define IOIRQ(avr,port,number) avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ(port),number)

#define DIRQLU(avr,n) IOIRQ(avr,PORT(n),PIN(n))