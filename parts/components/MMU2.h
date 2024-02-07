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
#include "IKeyClient.h"
#include "IScriptable.h"
#include "boards/MM_Control_01.h"  // for MM_Control_01
#include "sim_irq.h"               // for avr_irq_t
#include <atomic>
#include <cstdint>                // for uint32_t
#include <pthread.h>               // for pthread_t
#include <string>                  // for string


class MMU2: public BasePeripheral, public Boards::MM_Control_01, virtual private IKeyClient
{

    public:
        #define IRQPAIRS _IRQ(FEED_DISTANCE,"<mmu.feed_distance") _IRQ(RESET,"<mmu.reset") _IRQ(PULLEY_IN,"<mmu.pulley_in") \
                        _IRQ(SELECTOR_OUT,">sel_pos.out") _IRQ(IDLER_OUT,">idler_pos.out") _IRQ(LEDS_OUT,">leds.out") _IRQ(FINDA_OUT,">finda.out") \
						_IRQ(SHIFT_IN,"<32shift.in") _IRQ(SB_FS, ">sideband_fs.out") _IRQ(SB_BYTE_IN, "sideband_byte.in")
        #include "IRQHelper.h"

        // Creates a new MMU2. Creates board and starts it if bCreate = true
        explicit MMU2(bool bCreate = true, bool bSetupSideband = false);

		~MMU2() override {StopAVR();}

        // Returns the name of the serial port (for the pipe thread)
        const std::string GetSerialPort();

		inline void SetFINDAState(bool bVal) {m_bFINDAManual = bVal;}
		void ToggleFINDA();


    protected:

		static constexpr float FINDA_TRIGGER_DISTANCE = 33.f;
		static constexpr float FSENSOR_TRIGGER_DISTANCE = 400.f;

	#ifdef TEST_MODE
		friend void Test_MMU2_internal();
	#endif

		enum Actions
		{
			// We have to extend board rather than being our own IScriptable due
			// to it causing multiple-inheritance ambiguity.
			ActToggleFINDA = Board::ScriptAction::BOARD_ACT_END,
			ActSetFINDA,
			ActSetFINDAAuto
		};

		LineStatus ProcessAction(unsigned int iAction, const std::vector<std::string> &args) override;

        void SetupHardware() override;

		void OnKeyPress(const Key& key) override;

    private:

		void SetupSidebandControl();

        void OnResetIn(avr_irq_t *irq, uint32_t value);

        void OnPulleyFeedIn(avr_irq_t *irq, uint32_t value);

        void LEDHandler(avr_irq_t *irq, uint32_t value);

        void OnSidebandByteIn(avr_irq_t *irq, uint32_t value);

        std::atomic_bool m_bAutoFINDA = {true};
		std::atomic_bool m_bFINDAManual = {false};

		bool m_bSidebandEnabled = false;
		float m_fLastPosOut = 0;

        static MMU2 *g_pMMU; // Needed for GL
};
