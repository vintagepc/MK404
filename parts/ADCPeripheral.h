/*
	ADCPeripheral.h - Generalization helper for ADC-based peripherals.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

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

#pragma once

#include "BasePeripheral.h"
#include "avr_adc.h"

class ADCPeripheral: public BasePeripheral
{
    protected:
        // Returns the current mux number for this peripheral
        inline uint8_t GetMuxNumber() { return m_uiMux; }

        // Override this with your own ADC implementation. You don't need to worry abouy
        // verifying you are the current ADC channel.
        virtual uint32_t OnADCRead(struct avr_irq_t * irq, uint32_t value) = 0;

        // Sets up the IRQs on "avr" for this class. Optional name override IRQNAMES.
        template<class C>
        void _Init(avr_t *avr, uint8_t uiADC, C *p /*const char** IRQNAMES = nullptr*/) {
            BasePeripheral::_Init(avr,p);

            m_uiMux = uiADC;
			m_bConnected = true;

	        RegisterNotify(C::ADC_TRIGGER_IN, MAKE_C_CALLBACK(ADCPeripheral,_OnADCRead<C>), this);

            avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER); //NOLINT - complaint in external macro
            avr_irq_t * dst = avr_io_getirq(m_pAVR, AVR_IOCTL_ADC_GETIRQ, uiADC); //NOLINT - complaint in external macro
            if (src && dst)
			{
                ConnectFrom(src, C::ADC_TRIGGER_IN);
                ConnectTo(C::ADC_VALUE_OUT, dst);
            }
         };

		template<class C>
        void _SyncDigitalIRQ(uint32_t uiVOut)
        {
            if (uiVOut>2200) // 2.2V, logic H
			{
                RaiseIRQ(C::DIGITAL_OUT,1);
			}
            else if (uiVOut < 800) // 0.8v. L
			{
                RaiseIRQ(C::DIGITAL_OUT,0);
			}
            else
			{
                RaiseIRQFloat(C::DIGITAL_OUT,(m_pIrq.begin() + C::DIGITAL_OUT)->flags | IRQ_FLAG_FLOATING);
			}
        };

		inline bool IsConnected() { return m_bConnected; }
    private:
        template<class C>
        void _OnADCRead(struct avr_irq_t * irq, uint32_t value)
        {
          	union {
                avr_adc_mux_t v;
                uint32_t l;
            } u = { .l = value };
            avr_adc_mux_t v = u.v;

            if (v.src != m_uiMux)
			{
                return;
			}
            uint32_t uiVal = OnADCRead(irq,value);
            if (uiVal == m_uiLast)
			{
                return;
			}
            RaiseIRQ(C::ADC_VALUE_OUT,uiVal);
            _SyncDigitalIRQ<C>(uiVal);
            m_uiLast = uiVal;
        };

        uint8_t m_uiMux = 0;

        uint32_t m_uiLast = 0;

		bool m_bConnected = false;
};
