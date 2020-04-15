/*
    Simple fan tach sim for Einsy Rambo

*/
#include "heater.h"
#include "sim_irq.h"
#include "stdio.h"
#include "avr_ioport.h"
#include "math.h"

#define TRACE(_w)_w
#ifndef TRACE
#define TRACE(_w)
#endif

static const char * _heater_irq_names[IRQ_HEATER_COUNT] = {
	[IRQ_HEATER_PWM_IN] = "<heater.pwm.in",
    [IRQ_HEATER_DIGITAL_IN] = "<heater.digital.in",
	[IRQ_HEATER_TEMP_OUT] = ">heater.temp.out",
};

static avr_cycle_count_t
heater_temp_change(
	avr_t * avr,
	avr_cycle_count_t when,
	void * param)
{
    heater_t *this = (heater_t*) param;
    float fDelta = (this->fThermalMass*((float)(this->iPWM)/255.0f))*0.3;

    if (this->iPWM>0)
        this->fCurrentTemp += fDelta;
    else // Cooling - do a little exponential decay
    {
        float dT = (this->fCurrentTemp - this->fAmbientTemp)*pow(2.7183,-0.005*0.3);
        this->fCurrentTemp -= this->fCurrentTemp - (this->fAmbientTemp + dT);
    }
        

    TRACE(printf("New temp value: %.02f\n",this->fCurrentTemp));
    avr_raise_irq(this->irq + IRQ_HEATER_TEMP_OUT,(int)this->fCurrentTemp*256);

    if (this->iPWM>0 || this->fCurrentTemp>this->fAmbientTemp+0.2)
        avr_cycle_timer_register_usec(this->avr,300000,heater_temp_change,this);
    else
    {
        this->fCurrentTemp = this->fAmbientTemp;
         avr_raise_irq(this->irq + IRQ_HEATER_TEMP_OUT,(int)this->fCurrentTemp*256);
    }
        
    return 0;
}


static void heater_pwm_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param )
{
    heater_t *this = (heater_t*) param;

    if (this->flags.bAuto) // Only update RPM if auto (pwm-controlled). Else user supplied RPM.
        this->iPWM = value;
    TRACE(printf("New PWM value on heater: %u/\n", this->iPWM));
    if (this->iPWM > 0)
    {
        avr_cycle_timer_register_usec(this->avr,100000,heater_temp_change,this);
    }
  //  else
  //  {
  //      avr_cycle_timer_cancel(this->avr,fan_tach_change,this);
  //  }
}

static void heater_digital_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param )
{
    if (value==1)
        value = 255;
    heater_pwm_hook(irq,value,param);
}

void
heater_init(
		struct avr_t * avr,
        heater_t * this,
        float fThermalMass, 
        float fAmbientTemp,
        avr_irq_t *irqPWM,
        avr_irq_t *irqDigital)
{
    this->iPWM = 0x00;
    this->flags.bAuto = true;
    this->avr = avr;
    this->fThermalMass = fThermalMass;
    this->fCurrentTemp = fAmbientTemp;
    this->fAmbientTemp = fAmbientTemp;
    this->irq = avr_alloc_irq(&avr->irq_pool,0,IRQ_HEATER_COUNT,_heater_irq_names);
    if(irqPWM) avr_connect_irq(irqPWM, this->irq + IRQ_HEATER_PWM_IN);
    if(irqDigital) avr_connect_irq(irqDigital, this->irq + IRQ_HEATER_DIGITAL_IN);
    //avr_connect_irq(this->irq + IRQ_FAN_TACH_OUT,irqTach);

    //avr_irq_register_notify(this->irq + IRQ_HEATER_PWM_IN, heater_pwm_hook,this);
    avr_irq_register_notify(this->irq + IRQ_HEATER_DIGITAL_IN, heater_digital_hook,this);
}

void heater_set_pwm(heater_t *this, uint8_t iPWM)
{
    this->flags.bAuto = false;
    this->iPWM = iPWM;
    avr_raise_irq(this->irq + IRQ_HEATER_PWM_IN,0XFF);
}

void heater_resume_auto(heater_t *this)
{
    this->flags.bAuto = true;
}