/*
	PAT9125.h - Simulator for the MK3 optical filament sensor.

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

#include "I2CPeripheral.h"     // for I2CPeripheral
#include "IKeyClient.h"
#include "IScriptable.h"       // for IScriptable::LineStatus
#include "Scriptable.h"        // for Scriptable
#include "sim_avr.h"           // for avr_t
#include "sim_irq.h"           // for avr_irq_t
#include <cstdint>            // for uint8_t, uint32_t, int32_t, uint16_t
#include <string>              // for string
#include <vector>              // for vector

class PAT9125: public I2CPeripheral, public Scriptable, private IKeyClient
{

	public:
		#define IRQPAIRS \
			_IRQ(E_IN, "f<e_distance.in") \
			_IRQ(P_IN, "p_distance.in") \
			_IRQ(LED_OUT, "<presence.out")
		#include "IRQHelper.h"

		using FSState = enum {
			FS_MIN = -1,
			FS_NO_FILAMENT,
			FS_FILAMENT_PRESENT,
			FS_JAM,
			FS_AUTO, // Special state that only respects the auto value.
			FS_MAX
		};

		PAT9125();

		void Init(avr_t *pAVR, avr_irq_t *pSCL, avr_irq_t *pSDA);

		inline void Set(FSState eVal)
		{
			m_state = eVal;
			UpdateSensorState();
		}

		void ToggleJam();

		void Toggle();


	protected:
#ifdef TEST_MODE
		friend void Test_PAT9125_Toggle();
#endif
		void OnKeyPress(const Key& key) override;

		void UpdateSensorState();

		void OnPMotion(avr_irq_t *, uint32_t value);

		void OnEMotion(avr_irq_t *, uint32_t value);

		void SetYMotion(const float &fEVal, const float &fPVal);

		uint8_t GetRegVal(uint8_t uiAddr) override;

		bool SetRegVal(uint8_t uiAddr, uint32_t uiData) override;

		LineStatus ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs) override;

	private:

	enum Actions
	{
		ActToggle,
		ActSet,
		ActToggleJam,
		ActResumeAuto
	};

		float m_fYPos = 0.f, m_fPPos = 0.f, m_fEPos = 0.f;
		float m_fCurY = 0.f;
		union m_regs
		{
			uint8_t raw[32] {0x31, 0x91, 0, 0, 0, 0xA0, 0x17, 0,0, 0, 0x77, 0x10, 0, 0x14, 0x14, 0,0,0,0,0, 20, 0,0, 40,0, 0x04 };
			struct {
				uint8_t PID1;
				uint8_t PID2;
				uint8_t MStatus;
				uint8_t DXLow;
				uint8_t DYLow;
				uint8_t Mode;
				uint8_t Config;
				uint8_t :8;
				uint8_t :8;
				uint8_t WriteProtect;
				uint8_t Sleep1;
				uint8_t Sleep2;
				uint8_t :8;
				uint8_t Res_X;
				uint8_t Res_Y;
				uint8_t :8;
				uint8_t :8;
				uint8_t :8;
				uint8_t DeltaXYHi;
				uint8_t :8;
				uint8_t Shutter;
				uint8_t :8;
				uint8_t :8;
				uint8_t FrameAvg;
				uint8_t :8;
				uint8_t Orientation;
			};
		}m_regs;
		uint32_t m_uiRW = 0x2006E60; //1<<addr if RW.

		bool m_bFilament = false, m_bLoading = false;

		FSState m_state = FS_NO_FILAMENT;

		uint8_t m_uiNudgeCt = 0;

};
