/*
	Heater.h - a heater object for MK3Sim. There's not much to it,
    it just ticks the temperature "up" at a determined rate when active on PWM and down in
    in an exponential curve when off.

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


#pragma once

#include "BasePeripheral.h"
#include "Color.h"
#include "Scriptable.h"

class Heater : public BasePeripheral, public Scriptable
{
public:
    #define IRQPAIRS _IRQ(PWM_IN,"<heater.pwm_in") _IRQ(DIGITAL_IN,"<heater.digital_in") _IRQ(TEMP_OUT,">heater.temp_out") _IRQ(ON_OUT,">heater.on")
    #include "IRQHelper.h"


    // "Thermal mass" of the heater... deg C it heats/cools per sec at full-on (PWM=255);
    // Create a new heater with ambient fAmbientTemp, and thermal mass fThermalMass
    Heater(float fThermalMass, float fAmbientTemp, bool bIsBed, char chrLabel,
		   float fColdTemp, float fHotTemp);

    // Initializes the heater on "avr" and on irqPQM/irqDigital,
    void Init(avr_t *avr, avr_irq_t *irqPWM, avr_irq_t *irqDigital);

    // Overrides the auto PWM control. Useful for simulating a shorted or open MOSFET/heater.
    void Set(uint8_t uiPWM);

    // Returns to automatic control after having used Set()
    void Resume_Auto();

	// Draws the heater status
	void Draw();

	protected:
		Scriptable::LineStatus ProcessAction (unsigned int iAct, const vector<string> &vArgs) override;


    private:

		enum Actions
		{
			ActSetPWM,
			ActResume,
			ActStopHeating
		};

        // Hook for PWM change
        void OnPWMChanged(avr_irq_t *irq, uint32_t value);

        // Hook for digital full on/off
        void OnDigitalChanged(avr_irq_t *irq, uint32_t value);

        // Function for a temperature "tick" to add or remove heat
        avr_cycle_count_t OnTempTick(avr_t * avr, avr_cycle_count_t when);

        avr_cycle_timer_t m_fcnTempTick = MAKE_C_TIMER_CALLBACK(Heater,OnTempTick);

        bool m_bAuto = true;
        float m_fThermalMass = 1.0;
        float m_fAmbientTemp = 25.0;
        float m_fCurrentTemp = 25.0;
        bool m_bIsBed = false;
        char m_chrLabel;
        float m_fColdTemp;
        float m_fHotTemp;
        uint16_t m_uiPWM = 0;
		bool m_bStopTicking = false;
	    static constexpr Color3fv m_colColdTemp = {0, 1, 1};
	    static constexpr Color3fv m_colHotTemp = {1, 0, 0};
};
