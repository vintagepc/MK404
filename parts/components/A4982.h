/*
	A4982.h - Simulated Allegro driver used in the MiniRambo

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

#include "BasePeripheral.h"    // for MAKE_C_TIMER_CALLBACK
#include "sim_avr.h"           // for avr_t
#include "sim_avr_types.h"     // for avr_cycle_count_t
#include "sim_cycle_timers.h"  // for avr_cycle_timer_t
#include "sim_irq.h"           // for avr_irq_t

#include <atomic>
#include <cstdint>            // for uint8_t, uint32_t, int32_t, uint16_t
#include <string>               // for string

class A4982: public BasePeripheral
{
	public:
		#define IRQPAIRS \
			_IRQ(SLEEP_IN,       	"<A4982.sleep") \
			_IRQ(STEP_IN,       	"<A4982.step") \
			_IRQ(MS1_IN,	       	"<A4982.ms1") \
			_IRQ(MS2_IN,	       	"<A4982.ms2") \
			_IRQ(DIR_IN,            "<A4982.dir") \
			_IRQ(ENABLE_IN,         "<A4982.en") \
			_IRQ(RESET_IN,			"<A4982.reset") \
			_IRQ(MIN_OUT,	       	">A4982.min_endstop") \
			_IRQ(MAX_OUT,	       	">A4982.max_endstop") \
			_IRQ(POSITION_OUT,		">A4982.position") \
			_IRQ(STEP_POS_OUT,		">A4982.step_out")
		#include "IRQHelper.h"

		struct A4982_cfg_t {
			bool bInverted {false};
			uint16_t uiStepsPerMM {100};
			int16_t iMaxMM {200};
			float fStartPos {10.f};
			bool bHasNoEndStops {false};
		};

		// Default constructor.
		explicit A4982(char cAxis = ' ');

		// Sets the configuration to the provided values. (inversion, positions, etc)
		A4982_cfg_t& GetConfig() {return m_cfg;} ;

		void ReparseConfig();

		inline const std::string& GetName() { return m_strName;}

		// Registers with SimAVR.
		void Init(avr_t *avr);

		// Draws a simple visual representation of the motor position.
		void Draw();

		// Draws the position value as a number, without position ticks.
		void Draw_Simple();

	private:
		void _Draw(bool bSimple = false);

		void CheckEndstops();

		// Input handlers.
		void OnDirIn(avr_irq_t *irq, uint32_t value);
		void OnEnableIn(avr_irq_t *irq, uint32_t value);
		void OnMSIn(avr_irq_t *irq, uint32_t value);
		void OnResetIn(avr_irq_t *irq, uint32_t value);
		void OnSleepIn(avr_irq_t *irq, uint32_t value);
		void OnStepIn(avr_irq_t *irq, uint32_t value);

		avr_cycle_count_t OnWakeup(struct avr_t * avr, avr_cycle_count_t when);

		avr_cycle_timer_t m_fcnWakeup = MAKE_C_TIMER_CALLBACK(A4982,OnWakeup);

		bool m_bDir  = false;
		bool m_bReset = false;
		std::atomic_bool m_bEnable {false}, m_bSleep {false}, m_bConnected {false};


		int32_t m_iCurStep = 0;
		int32_t m_iMaxPos = 0;
		uint8_t m_uiStepSize = 16;
		std::atomic<float> m_fCurPos = {0}, m_fEnd = {0}; // Tracks position in float for gl
		std::atomic_char m_cAxis {' '};

		A4982_cfg_t m_cfg = A4982_cfg_t();

		std::string m_strName {"A4982_"};

		// Position helpers
		float StepToPos(int32_t step);
		int32_t PosToStep(float step);
};
