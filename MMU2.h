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


#include "BasePeripheral.h"
#include "boards/MM_Control_01.h"

class MMU2: public BasePeripheral, public Boards::MM_Control_01
{

    public:
        #define IRQPAIRS _IRQ(FEED_DISTANCE,"<mmu.feed_distance") _IRQ(RESET,"<mmu.reset") _IRQ(PULLEY_IN,"<mmu.pulley_in") \
                        _IRQ(SELECTOR_OUT,">sel_pos.out") _IRQ(IDLER_OUT,">idler_pos.out") _IRQ(LEDS_OUT,">leds.out")
        #include "IRQHelper.h"

        // Creates a new MMU2. Does all of the setup and firmware load.
        MMU2();

		~MMU2(){StopAVR();}

        // Returns the name of the serial port (for the pipe thread)
        std::string GetSerialPort();

        void Draw();

    protected:
        void SetupHardware() override;

    private:

        void* Run();

        void OnResetIn(avr_irq_t *irq, uint32_t value);

        void OnPulleyFeedIn(avr_irq_t *irq, uint32_t value);

        void LEDHandler(avr_irq_t *irq, uint32_t value);

        bool m_bQuit = false;
        bool m_bStarted = false;
        bool m_bReset = false;
        pthread_t m_tRun;

        std::string m_strTitle = "Missing Material Unit 2";

        static MMU2 *g_pMMU; // Needed for GL
};




#endif
