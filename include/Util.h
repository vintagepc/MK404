/*

    Util.h - convenience macros for working with SimAVR.

*/

#ifndef __UTIL_H__
#define __UTIL_H__

#include "PinHelper_2560.h"
#include "avr_ioport.h"
#include "avr_timer.h"

inline static unsigned int _TimerPWMID(uint8_t number) { return TIMER_IRQ_OUT_PWM0 + number; }

// Shorthand to get IRQ for a digital IO port.
inline static struct avr_irq_t* IOIRQ(struct avr_t* avr,uint8_t port,uint8_t number) { return avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ(port),number); }

// Timer PWM shorthand function
inline static struct avr_irq_t* TIMERIRQ(struct avr_t* avr,uint8_t timer,uint8_t number)	{return avr_io_getirq(avr,AVR_IOCTL_TIMER_GETIRQ(timer), _TimerPWMID(number)); }

// Looks up a digital IRQ based on the arduino convenience pin number.
inline static struct avr_irq_t* DIRQLU(struct avr_t* avr, unsigned int n){ return IOIRQ(avr,PORT(n),PIN(n)); }

// Looks up a PWM IRQ based on the arduino convenience pin number.
inline static struct avr_irq_t* DPWMLU(struct avr_t* avr, unsigned int n) { return TIMERIRQ(avr, TIMER_CHAR(n), TIMER_IDX(n)); } 



//typedef void (*voidCCallback)();


// // Generates a C function compatible with a class member function.
// #define MAKE_IRQ_C_FUNC(classname,funcname) static void classname##_##funcname(struct avr_irq_t * irq, uint32_t value, void *param) \
//     { \
//         classname *p = (classname*)param; \
//         p->funcname(irq,value); \
//     }

#endif