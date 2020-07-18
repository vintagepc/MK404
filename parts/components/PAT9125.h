/*
	PAT9125.h - Simulator for the MK3 optical filament sensor.

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

#include <stdint.h>            // for uint8_t, uint32_t, int32_t, uint16_t
#include <string>              // for string
#include <vector>              // for vector
#include <map>
#include "BasePeripheral.h"    // for MAKE_C_TIMER_CALLBACK
#include "IScriptable.h"       // for IScriptable::LineStatus
#include "I2CPeripheral.h"     // for I2CPeripheral
#include "Scriptable.h"        // for Scriptable
#include "sim_avr.h"           // for avr_t
#include "sim_avr_types.h"     // for avr_cycle_count_t
#include "sim_cycle_timers.h"  // for avr_cycle_timer_t
#include "sim_irq.h"           // for avr_irq_t

class PAT9125: public I2CPeripheral, public Scriptable
{

	public:
		#define IRQPAIRS \
	        _IRQ(TX_IN,       "32<PAT9125.i2c_in") \
	        _IRQ(TX_REPLY,  	"8>PAT9125.byte_out") \
			_IRQ(E_IN, "f<e_distance.in") \
			_IRQ(LED_OUT, "<presence.out")
		#include "IRQHelper.h"

		PAT9125():I2CPeripheral(0x75),Scriptable("PAT9125")
		{
		};

		void Init(avr_t *pAVR, avr_irq_t *pSCL, avr_irq_t *pSDA)
		{
			_Init(pAVR, pSDA, pSCL, this);
			printf("\n\n--------- Your attention please! ----------\n");
			printf("NOTE: PAT9125 is not functional due to a sorely lacking datasheet.\nIf you are familiar with this sensor\n please consider contributing to an implementation.\n");
			printf("--------- Your attention please! ----------\n\n\n");
			RegisterNotify(E_IN, MAKE_C_CALLBACK(PAT9125,OnEMotion),this);
		}

		inline void Toggle()
		{
			m_bLoading = !m_bFilament;
			m_bFilament^=1;
			printf("Filament Present: %u\n",m_bFilament);
			RaiseIRQ(LED_OUT,!m_bFilament); // LED is inverted.
			if (m_bFilament)
			{
				m_regs.Shutter = 5; // Restore shutter/brightness.
				m_regs.FrameAvg = 100;
			}
			else
			{
				m_regs.Shutter = 20; // drop shutter as if underexposed.
				m_regs.FrameAvg = 40; // and brightness.
			}
			m_regs.MStatus = 0x80*m_bFilament;

		}

	protected:
		void OnEMotion(avr_irq_t *pIRQ, uint32_t value)
		{
			m_bLoading=false; // clear loading flag once E move started.
			if (m_bFilament)
			{

				float *fV = reinterpret_cast<float*>(&value);
				SetYMotion(fV[0]);
				// (5*PAT9125_YRES/25.4)

				//m_regs.MStatus = 0x80;
			}
			else // Clear motion and regs.
			{
				m_regs.MStatus = 0;
				m_regs.DeltaXYHi &= 0xF0;
				m_regs.DYLow = 0;
			}
		}

		void SetYMotion(const float &fVal)
		{
				float fDelta = fVal-m_fYPos;
				int16_t iCounts  = fDelta*(5.f*(float)m_regs.Res_Y/25.4f);
				iCounts = -iCounts;
				m_fCurY = fVal;
				m_regs.DeltaXYHi = (iCounts >> 8) & 0b1111;
				m_regs.DYLow = iCounts & 0xFF;
		}

		virtual uint8_t GetRegVal(uint8_t uiAddr) override
		{
			switch (uiAddr)
			{
				case 0x02:
				{
					uint8_t val = m_regs.MStatus;
					if (!m_bLoading)
						m_regs.MStatus = 0; // clear motion flag.
					return val;
				}
				case 0x04:
				{
					if (m_bLoading)
						SetYMotion(m_fCurY += 1.f);
					m_fYPos = m_fCurY;
				}
				/* FALLTHRU */
				default:
					//printf("Read: %02x, %02x\n",uiAddr, m_regs.raw[uiAddr]);
					return m_regs.raw[uiAddr];
			}
		};

		virtual bool SetRegVal(uint8_t uiAddr, uint32_t uiData)
		{
			if (!(m_uiRW  & (0x01<<uiAddr)))
			{
				printf("PAT9125: tried to write Read-only register\n");
				return false; // RO register.
			}
			m_regs.raw[uiAddr] = uiData & 0xFF;
			//printf("Wrote: %02x = %02x (%02x)\n",uiAddr,uiData, m_regs.raw[uiAddr]);
			return true;
		};

	private:
		uint8_t m_uiAddr = 0;
		float m_fYPos = 0.f;
		float m_fCurY = 0.f;
		union m_regs
		{
			m_regs()
			{
				for (int i=0; i<32; i++)
					raw[i] = 0;
				PID1 = 0x31;
				PID2 = 0x91;
				Mode = 0xA0;
				Config = 0x17;
				Sleep1 = 0x77;
				Sleep2 = 0x10;
				Res_X = Res_Y = 0x14;
				Orientation = 0x04;
				FrameAvg = 100;
				Shutter = 5;
			};
			uint8_t raw[32];
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

};
