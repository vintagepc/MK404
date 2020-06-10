/*
	VoltageSrc.h - a voltage src ADC peripheral.

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


#ifndef __VOLTAGESRC_H___
#define __VOLTAGESRC_H___

#include <string.h>
#include <ADCPeripheral.h>

class VoltageSrc: public ADCPeripheral {
public:

    // Macro to define a set of IRQs and string names. 
    #define IRQPAIRS    _IRQ(ADC_TRIGGER_IN,"8<voltage.trigger") \
                        _IRQ(ADC_VALUE_OUT, "16>voltage.value_out")\
                        _IRQ(VALUE_IN, "16<voltage.value_in")\
                        _IRQ(DIGITAL_OUT, ">voltage.digital_out")

    // Helper to keep pairs in sync.
    #include <IRQHelper.h>

    // Constructs a new VoltageSrc on ADC mux uiMux, with a v scale factor of fVScale and a starting reading of fStartV
    VoltageSrc(float fVScale = 1.0f, // voltage scale factor to bring it in line with the ADC 0-5v input.
            float fStartV = 0.0f );

    // Initializes the source (connets it to supplied AVR's ADC)
    void Init(struct avr_t * avr, uint8_t uiMux);

    // Changes the voltage reading to fVal
    void Set(float fVal);

protected:
    // ADC read trigger. 
    virtual uint32_t OnADCRead(avr_irq_t *pIRQ, uint32_t value) override;

    // Input trigger
    void OnInput(avr_irq_t *pIRQ, uint32_t value);

    float m_fVScale = 1.0f;
    float m_fCurrentV = 0.0f;
};

#endif /* __VOLTAGESRC_H___ */
