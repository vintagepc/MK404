/*

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __VOLTAGESRC_H___
#define __VOLTAGESRC_H___

#include <string.h>
#include <BasePeripheral.h>

class VoltageSrc: public BasePeripheral {
public:
    #define IRQPAIRS    _IRQ(ADC_TRIGGER_IN,"8<voltage.trigger") \
                        _IRQ(ADC_VALUE_OUT, "16>voltage.value_out")\
                        _IRQ(VALUE_IN, "16<voltage.value_in")\
                        _IRQ(DIGITAL_OUT, ">voltage.digital_out")

    #include <IRQHelper.h>

    VoltageSrc(uint8_t uiMux,
            float fVScale = 1.0f, // voltage scale factor to bring it in line with the ADC 0-5v input.
            float fStartV = 0.0f );

    virtual void OnADCRead(avr_irq_t *pIRQ, uint32_t value);
    void OnInput(avr_irq_t *pIRQ, uint32_t value);
    void Init(struct avr_t * avr);
    void Set(float fVal);

protected:
    void SendToADC(uint32_t uiVOut);
    struct avr_t *m_pAVR = nullptr;
    uint8_t m_uiMuxNr = -1;
    float m_fVScale = 1.0f;
    float m_fCurrentV = 0.0f;
};

#endif /* __VOLTAGESRC_H___ */
