/*
	ADCPeripheral.h - Generalization helper for ADC-based peripherals.

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


#include "BasePeripheral.h"
#include <avr_adc.h>

class ADCPeripheral: public BasePeripheral
{
    protected:
        void OnADCReadGuard(struct avr_irq_t * irq, uint32_t value)
        {
          	union {
                avr_adc_mux_t v;
                uint32_t l;
            } u = { .l = value };
            avr_adc_mux_t v = u.v;

            if (v.src != m_uiMux)
                return;  
            OnADCRead(irq,value);
        };

        virtual void OnADCRead(struct avr_irq_t * irq, uint32_t value) = 0;

        // Sets up the IRQs on "avr" for this class. Optional name override IRQNAMES.
        template<class C>
        void _Init(avr_t *avr, uint8_t uiADC, avr_irq_notify_t fcn, C *p, const char** IRQNAMES = nullptr) {
            BasePeripheral::_Init(avr,p);

            m_uiMux = uiADC;

	        RegisterNotify(C::ADC_TRIGGER_IN, fcn, this);

            avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER);
            avr_irq_t * dst = avr_io_getirq(m_pAVR, AVR_IOCTL_ADC_GETIRQ, uiADC);
            if (src && dst) {
                ConnectFrom(src, C::ADC_TRIGGER_IN);
                ConnectTo(C::ADC_VALUE_OUT, dst);
            }
         };
         
        uint8_t m_uiMux = 0;
};
