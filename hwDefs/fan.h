/*
    Simple fan tach sim for Einsy Rambo

*/

#ifndef __FAN_H__
#define __FAN_H__

#include "sim_irq.h"
#include "stdbool.h"

enum {
	IRQ_FAN_PWM_IN = 0,
    IRQ_FAN_DIGITAL_IN,
    IRQ_FAN_SPEED_OUT,
    IRQ_FAN_TACH_OUT,
	IRQ_FAN_COUNT
};

typedef struct fan_flags_t
{
    uint8_t bStalled : 1;
    uint8_t bAuto : 1;
}fan_flags_t;


typedef struct fan_t {
	avr_irq_t * irq;	// output irq
	struct avr_t * avr;
	uint8_t iPWM;
    uint16_t iMaxRPM;
    uint16_t iCurrentRPM;
    uint16_t iUsecPulse;
    bool bPulseState;
    fan_flags_t flags;
} fan_t;


void
fan_init(
		struct avr_t * avr,
		fan_t * b,
		uint16_t iMaxRPM,
        avr_irq_t *irqTach,
        avr_irq_t *irqDigital,
        avr_irq_t *irqPWM);

void fan_stall(fan_t *p, bool bStall);
void fan_set_rpm(fan_t *this, uint16_t iRPM);
void fan_resume_auto(fan_t *this);
#endif /* __BUTTON_H__*/
