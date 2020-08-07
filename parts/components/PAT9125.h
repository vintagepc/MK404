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
			_IRQ(P_IN, "p_distance.in") \
			_IRQ(LED_OUT, "<presence.out")
		#include "IRQHelper.h"

		typedef enum FSState {
			FS_MIN = -1,
			FS_NO_FILAMENT,
			FS_FILAMENT_PRESENT,
			FS_JAM,
			FS_AUTO, // Special state that only respects the auto value.
			FS_MAX
		}FSState_t;

		PAT9125():I2CPeripheral(0x75),Scriptable("PAT9125")
		{
			RegisterActionAndMenu("Toggle","Toggles the IR sensor state",ActToggle);
			RegisterAction("Set","Sets the sensor state to a specific enum entry. (int value)",ActSet,{ArgType::Int});
			RegisterMenu("Toggle Filament jam",ActToggleJam);
			RegisterMenu("Resume Auto",ActResumeAuto);
		};

		void Init(avr_t *pAVR, avr_irq_t *pSCL, avr_irq_t *pSDA)
		{
			_Init(pAVR, pSDA, pSCL, this);
			printf("\n\n--------- Your attention please! ----------\n");
			printf("NOTE: PAT9125 is minimally functional. If you encounter issues or need advanced functionality \n feel free to contribute or open an issue.\n");
			printf("--------- Your attention please! ----------\n\n\n");
			RegisterNotify(E_IN, MAKE_C_CALLBACK(PAT9125,OnEMotion),this);
			RegisterNotify(P_IN, MAKE_C_CALLBACK(PAT9125,OnPMotion),this);
		}

		inline void Set(FSState eVal)
		{
			m_state = eVal;
			UpdateSensorState();
		}

		inline void ToggleJam()
		{
			if (m_state == FS_JAM)
				m_state = FS_FILAMENT_PRESENT;
			else
				m_state = FS_JAM;
			printf("PAT9125 Jam: %u\n", m_state == FS_JAM);

		}

		void Toggle()
		{
			switch (m_state)
			{
			case FS_AUTO:
				printf("Leaving PAT9125 Auto mode\n");
				m_state = m_bFilament ? FS_FILAMENT_PRESENT : FS_NO_FILAMENT;
				/* FALLTHRU */  // Deliberate fallthrough - will toggle from current.
			case FS_NO_FILAMENT:
				m_bLoading = true; // Load from no filament only.
				/* FALLTHRU */
			case FS_JAM:
				m_state = FS_FILAMENT_PRESENT;
				UpdateSensorState();
				break;
			case FS_FILAMENT_PRESENT:
				m_state = FS_NO_FILAMENT;
				UpdateSensorState();
				break;
			default: // No action
				break;
			}
		}


	protected:

		void UpdateSensorState()
		{
			if (m_state != FS_AUTO)
				m_bFilament = m_state == FS_FILAMENT_PRESENT || m_state == FS_JAM;
			printf("Filament Present: %u\n",m_bFilament);
			RaiseIRQ(LED_OUT,!m_bFilament); // LED is inverted.
			if (m_bFilament)
			{
				m_regs.Shutter = 5; // Restore shutter/brightness.
				m_regs.FrameAvg = 100;
				m_uiNudgeCt = 0;
			}
			else
			{
				m_regs.Shutter = 20; // drop shutter as if underexposed.
				m_regs.FrameAvg = 40; // and brightness.
			}
		}

		void OnPMotion(avr_irq_t *pIRQ, uint32_t value)
		{
			m_bLoading=false; // clear loading flag once E move started.
			auto fVal = reinterpret_cast<float*>(&value);
			bool bLoaded = fVal[0]>370.f;
			if (m_state == FS_AUTO) // Set filament state if auto.
			{
				if (m_bFilament != bLoaded)
				{
					m_bFilament = bLoaded; // default length is 380, trip a few mm earlier because that's where the sensor is vs gears.
					UpdateSensorState();
				}
			}
			if (bLoaded) // Pass through if fed enough to reach.
				SetYMotion(m_fEPos,fVal[0]-370.f);


		}
		void OnEMotion(avr_irq_t *pIRQ, uint32_t value)
		{
			m_bLoading=false; // clear loading flag once E move started.
			if (m_bFilament)
			{
				float *fV = reinterpret_cast<float*>(&value);
				SetYMotion(fV[0], m_fPPos);
			}
			else // Clear motion and regs.
			{
				m_regs.MStatus = 0;
				m_regs.DeltaXYHi &= 0xF0;
				m_regs.DYLow = 0;
			}
		}

		void SetYMotion(const float &fEVal, const float &fPVal)
		{
				//printf("YMotion update: %f %f\n",fEVal, fPVal);
				float fDelta = (fEVal+fPVal)-m_fYPos;
				int16_t iCounts  = fDelta*(5.f*(float)m_regs.Res_Y/25.4f);
				iCounts = -iCounts;
				if (fDelta>0 && iCounts==0) // Enforce minimum motion of at least 1 count.
					iCounts = -1;
				else if (fDelta<0 && iCounts==0)
					iCounts = 1;
				m_fEPos = fEVal;
				m_fPPos = fPVal;
				m_fCurY = fEVal+fPVal;
				m_regs.DeltaXYHi = (iCounts >> 8) & 0b1111;
				m_regs.DYLow = iCounts & 0xFF;
				if (m_state != FS_JAM)
					m_regs.MStatus = 0x80;

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
					else
					{
						SetYMotion(m_fCurY += 1.f,m_fPPos);
						m_uiNudgeCt++;
						if (m_uiNudgeCt>4)
						{
							m_uiNudgeCt = 0;
							m_bLoading = false;
						}
					}
					return val;
				}
				case 0x04:
				{
					printf("Read DY: %d (%f) \n",m_regs.raw[uiAddr], (m_fYPos-m_fCurY));
					m_fYPos = m_fCurY;
				}
				/* FALLTHRU */
				default:
					//printf("Read: %02x, %02x\n",uiAddr, m_regs.raw[uiAddr]);
					return m_regs.raw[uiAddr];
			}
		};

		virtual bool SetRegVal(uint8_t uiAddr, uint32_t uiData) override
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

		LineStatus ProcessAction(unsigned int iAct, const vector<string> &vArgs) override
		{
			switch (iAct)
			{
				case ActToggle:
					Toggle();
					return LineStatus::Finished;
				case ActSet:
				{
					int iVal = stoi(vArgs.at(0));
					if (iVal<0 || iVal >= FSState::FS_MAX)
						return IssueLineError(string("Set value ") + to_string(iVal) + " is out of the range [0,3]" );
					Set((FSState)iVal);
					return LineStatus::Finished;
				}
				case ActToggleJam:
					ToggleJam();
					return LineStatus::Finished;
				case ActResumeAuto:
					Set(FS_AUTO);
					return LineStatus::Finished;
			}
			return LineStatus::Unhandled;
		}

	private:

	enum Actions
	{
		ActToggle,
		ActSet,
		ActToggleJam,
		ActResumeAuto
	};

		// uint8_t m_uiAddr = 0;                            // not used?
		float m_fYPos = 0.f, m_fPPos = 0.f, m_fEPos = 0.f;
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
				FrameAvg = 40; // "no filament" default.
				Shutter = 20;// 5;
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

		FSState m_state = FS_NO_FILAMENT;

		uint8_t m_uiNudgeCt = 0;

};
