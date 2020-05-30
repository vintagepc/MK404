/*
	MMU2.h - A Missing-materials-unit for MK404

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

#ifndef __MMU_H___
#define __MMU_H___

#include <pthread.h>

#include "uart_pty.h"
#include "ADC_Buttons.h"
#include "HC595.h"
#include "LED.h"
#include "TMC2130.h"

#include "BasePeripheral.h"

class MMU2: public BasePeripheral
{

    public:
        #define IRQPAIRS _IRQ(FEED_DISTANCE,"<mmu.feed_distance") _IRQ(RESET,"<mmu.reset") _IRQ(PULLEY_IN,"<mmu.pulley_in") \
                        _IRQ(SELECTOR_OUT,">sel_pos.out") _IRQ(IDLER_OUT,">idler_pos.out") _IRQ(LEDS_OUT,">leds.out")
        #include "IRQHelper.h"

        // Creates a new MMU2
        MMU2();

        // Initializes the AVR and loads the firmware.
        void Init();

        // Actually starts the MMU processing thread.
        void Start();

        // Creates the GL window and context for the visuals.
        void StartGL();

        // Shuts down the MMU thread.
        void Stop();

        // Returns the name of the serial port (for the pipe thread)
        std::string GetSerialPort();

    private:

        void* Run();

        void Draw();

        void InitGL();

        void OnDisplayTimer(int i);

        void OnResetIn(avr_irq_t *irq, uint32_t value);

        void LEDHandler(avr_irq_t *irq, uint32_t value);
        
        void OnPulleyFeedIn(avr_irq_t *irq, uint32_t value);

        void SetupHardware();

        bool m_bQuit = false;
        bool m_bStarted = false;
        bool m_bReset = false;
        pthread_t m_tRun;
        uart_pty m_UART0;
        HC595 m_shift;
        TMC2130 m_Sel, m_Idl, m_Extr;
        LED m_lGreen[5], m_lRed[5], m_lFINDA;
        ADC_Buttons m_buttons;
        int m_iWindow = 0, m_iWinW = 0, m_iWinH = 0;

        static MMU2 *g_pMMU; // Needed for GL
};




#endif