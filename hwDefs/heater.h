/*
    Simple fan tach sim for Einsy Rambo

*/

#ifndef __HEATER_H__
#define __HEATER_H__

#include "sim_irq.h"
#include "stdbool.h"

enum {
	IRQ_HEATER_PWM_IN = 0,
    IRQ_HEATER_DIGITAL_IN,
    IRQ_HEATER_TEMP_OUT,
	IRQ_HEATER_COUNT
};

typedef struct heater_flags_t
{
    uint8_t bAuto : 1;
}heater_flags_t;


typedef struct heater_t {
	avr_irq_t * irq;	// output irq
	struct avr_t * avr;
	float fCurrentTemp;
    float fAmbientTemp;
    uint16_t iPWM;
    float fThermalMass;
    heater_flags_t flags;
} heater_t;


void
heater_init(
		struct avr_t * avr,
		heater_t * b,
		float fThermalMass, // "Thermal mass" of the heater... deg C it heats/cools per sec at full-on (PWM=255);
        float fAmbientTemp,
        avr_irq_t *irqPWM,
        avr_irq_t *irqDigital);

void heater_set_pwm(heater_t *this, uint8_t iPWM); // Use this to override PWM, e.g. shorted or open MOSFET
void heater_resume_auto(heater_t *this); //Resume auto control after using set_pwm.
#endif /* __HEATER_H__*/
