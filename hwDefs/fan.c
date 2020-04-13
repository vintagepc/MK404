/*
    Simple fan tach sim for Einsy Rambo

*/
#include "fan.h"
#include "sim_irq.h"
#include "stdio.h"
#include "avr_ioport.h"

#define TRACE(_w)_w
#ifndef TRACE
#define TRACE(_w)
#endif

static const char * _fan_irq_names[IRQ_FAN_COUNT] = {
	[IRQ_FAN_PWM_IN] = "<fan_pwm.in",
	[IRQ_FAN_TACH_OUT] = ">fan_tach.out",
	[IRQ_FAN_SPEED_OUT] = ">fan_speed.out",
};


static avr_cycle_count_t
fan_tach_change(
	avr_t * avr,
	avr_cycle_count_t when,
	void * param)
{
    fan_t *this = (fan_t*) param;
    avr_raise_irq(this->irq + IRQ_FAN_TACH_OUT,this->bPulseState^=1);
    avr_cycle_timer_register_usec(this->avr,this->iUsecPulse,fan_tach_change,this);
    return 0;
}

static void fan_pwm_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param )
{
    fan_t *this = (fan_t*) param;
    this->iPWM = value;
    if (this->flags.bAuto) // Only update RPM if auto (pwm-controlled). Else user supplied RPM.
        this->iCurrentRPM = ((this->iMaxRPM)*value)/255;
    avr_raise_irq(this->irq + IRQ_FAN_SPEED_OUT,this->iCurrentRPM);
    float fSecPerRev = 60.0f/(float)this->iCurrentRPM;
    float fuSPerRev = 1000000*fSecPerRev;
    this->iUsecPulse = fuSPerRev/4; // 4 pulses per rev.
    TRACE(printf("New PWM/RPM/cyc: %u / %u / %u\n",this->iPWM, this->iCurrentRPM, this->iUsecPulse));
    if (this->iCurrentRPM>0)
    {
        avr_cycle_timer_register_usec(this->avr,this->iUsecPulse,fan_tach_change,this);
    }
    else
    {
        avr_cycle_timer_cancel(this->avr,fan_tach_change,this);
    }
}

void
fan_init(
		struct avr_t * avr,
        fan_t * this,
		uint16_t iMaxRPM,
        avr_irq_t *irqTach,
        avr_irq_t *irqPWM)
{
    this->iMaxRPM = iMaxRPM;
    this->flags.bStalled = false;
    this->flags.bAuto = true;
    this->avr = avr;
    this->irq = avr_alloc_irq(&avr->irq_pool,0,IRQ_FAN_COUNT,_fan_irq_names);
    if(irqPWM) avr_connect_irq(irqPWM, this->irq + IRQ_FAN_PWM_IN);
    avr_connect_irq(this->irq + IRQ_FAN_TACH_OUT,irqTach);

    avr_irq_register_notify(this->irq + IRQ_FAN_PWM_IN, fan_pwm_hook,this);
}

void fan_stall(fan_t *p, bool bStall)
{
    p->flags.bStalled = bStall;
}

void fan_set_rpm(fan_t *this, uint16_t iRPM)
{
    this->flags.bAuto = false;
    this->iCurrentRPM = iRPM;
    avr_raise_irq(this->irq + IRQ_FAN_PWM_IN,0XFF);
}

void fan_resume_auto(fan_t *this)
{
    this->flags.bAuto = true;
}