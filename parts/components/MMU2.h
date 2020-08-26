/*
	MMU2.h - A Missing-materials-unit for MK404

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

#include "ADC_Buttons.h"
#include "BasePeripheral.h"        // for BasePeripheral
#include "boards/MM_Control_01.h"  // for MM_Control_01
#include "sim_irq.h"               // for avr_irq_t
#include <atomic>
#include <cstdint>                // for uint32_t
#include <pthread.h>               // for pthread_t
#include <string>                  // for string


class MMU2: public BasePeripheral, public Boards::MM_Control_01
{

    public:
        #define IRQPAIRS _IRQ(FEED_DISTANCE,"<mmu.feed_distance") _IRQ(RESET,"<mmu.reset") _IRQ(PULLEY_IN,"<mmu.pulley_in") \
                        _IRQ(SELECTOR_OUT,">sel_pos.out") _IRQ(IDLER_OUT,">idler_pos.out") _IRQ(LEDS_OUT,">leds.out") _IRQ(FINDA_OUT,">finda.out")
        #include "IRQHelper.h"

        // Creates a new MMU2. Does all of the setup and firmware load.
        MMU2();

		~MMU2(){StopAVR();}

        // Returns the name of the serial port (for the pipe thread)
        const std::string GetSerialPort();

        void Draw(float fY);

		inline void PushButton(uint8_t uiBtn) { m_buttons.Push(uiBtn);}
		inline void SetFINDAAuto(bool bVal) { m_bAutoFINDA = bVal;}
		inline void SetFINDAState(bool bVal) {m_bFINDAManual = bVal;}
		void ToggleFINDA();

    protected:
        void SetupHardware() override;

    private:

        void* Run();

        void OnResetIn(avr_irq_t *irq, uint32_t value);

        void OnPulleyFeedIn(avr_irq_t *irq, uint32_t value);

        void LEDHandler(avr_irq_t *irq, uint32_t value);

        atomic_bool m_bAutoFINDA = {true};
		atomic_bool m_bFINDAManual = {false};
        atomic_bool m_bStarted = {false};
        atomic_bool m_bReset ={false};
        pthread_t m_tRun = 0;

        std::string m_strTitle = "Missing Material Unit 2";

        static MMU2 *g_pMMU; // Needed for GL
};
