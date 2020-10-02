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
#include "PAT9125.h"
#include "BasePeripheral.h"    // for MAKE_C_TIMER_CALLBACK
#include "I2CPeripheral.h"     // for I2CPeripheral
#include "IKeyClient.h"
#include "IScriptable.h"       // for IScriptable::LineStatus
#include "Scriptable.h"        // for Scriptable
#include "gsl-lite.hpp"
#include "sim_avr.h"           // for avr_t
#include "sim_irq.h"           // for avr_irq_t
#include <cstdint>            // for uint8_t, uint32_t, int32_t, uint16_t
#include <cstring>
#include <iostream>
#include <string>              // for string
#include <vector>              // for vector


PAT9125::PAT9125():I2CPeripheral(0x75),Scriptable("PAT9125"),IKeyClient()
{
	// Check register packing/sizes:
	Expects(sizeof(m_regs) == sizeof(m_regs.raw));

	RegisterActionAndMenu("Toggle","Toggles the IR sensor state",ActToggle);
	RegisterAction("Set","Sets the sensor state to a specific enum entry. (int value)",ActSet,{ArgType::Int});
	RegisterActionAndMenu("Toggle Jam","Toggles a jam (motion stall)",ActToggleJam);
	RegisterActionAndMenu("Resume Auto","Resumes auto (MMU-pulley-based) operation",ActResumeAuto);

	RegisterKeyHandler('f', "Toggle PAT9125 filament presence");
	RegisterKeyHandler('j', "Toggle a simulated jam on the PAT9125");
	RegisterKeyHandler('A',"");

};

void PAT9125::Init(avr_t *pAVR, avr_irq_t *pSCL, avr_irq_t *pSDA)
{
	_InitWithArgs(pAVR,this,nullptr, pSDA, pSCL);
	std::cout << "\n\n--------- Your attention please! ----------\n";
	std::cout << "NOTE: PAT9125 is minimally functional. If you encounter issues or need advanced functionality \n feel free to contribute or open an issue.\n";
	std::cout << "--------- Your attention please! ----------\n\n\n";
	RegisterNotify(E_IN, MAKE_C_CALLBACK(PAT9125,OnEMotion),this);
	RegisterNotify(P_IN, MAKE_C_CALLBACK(PAT9125,OnPMotion),this);
}


void PAT9125::ToggleJam()
{
	if (m_state == FS_JAM)
	{
		m_state = FS_FILAMENT_PRESENT;
	}
	else
	{
		m_state = FS_JAM;
	}
	std::cout << "PAT9125 Jam: " << (m_state == FS_JAM) << '\n';

}

void PAT9125::Toggle()
{
	switch (m_state)
	{
	case FS_AUTO:
		std::cout << "Leaving PAT9125 Auto mode\n";
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

void PAT9125::OnKeyPress(const Key& key)
{
	switch(key)
	{
		case 'f':
			Toggle();
			break;
		case 'j':
			ToggleJam();
			break;
		case 'A':
			Set(PAT9125::FS_AUTO);
			break;
	}
}

void PAT9125::UpdateSensorState()
{
	if (m_state != FS_AUTO)
	{
		m_bFilament = m_state == FS_FILAMENT_PRESENT || m_state == FS_JAM;
	}
	std::cout << "Filament Present: " << m_bFilament << '\n';
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

void PAT9125::OnPMotion(avr_irq_t *, uint32_t value)
{
	m_bLoading=false; // clear loading flag once E move started.
	float fVal;
	std::memcpy(&fVal,&value, sizeof(value));
	bool bLoaded = fVal>370.f;
	if (m_state == FS_AUTO) // Set filament state if auto.
	{
		if (m_bFilament != bLoaded)
		{
			m_bFilament = bLoaded; // default length is 380, trip a few mm earlier because that's where the sensor is vs gears.
			UpdateSensorState();
		}
	}
	if (bLoaded) // Pass through if fed enough to reach.
	{
		SetYMotion(m_fEPos,fVal-370.f);
	}


}
void PAT9125::OnEMotion(avr_irq_t *, uint32_t value)
{
	m_bLoading=false; // clear loading flag once E move started.
	if (m_bFilament)
	{
		float fV;
		std::memcpy(&fV, &value, sizeof(value));
		SetYMotion(fV, m_fPPos);
	}
	else // Clear motion and regs.
	{
		m_regs.MStatus = 0;
		m_regs.DeltaXYHi &= 0xF0;
		m_regs.DYLow = 0;
	}
}

void PAT9125::SetYMotion(const float &fEVal, const float &fPVal)
{
		//printf("YMotion update: %f %f\n",fEVal, fPVal);
		float fDelta = (fEVal+fPVal)-m_fYPos;
		int16_t iCounts  = fDelta*(5.f*static_cast<float>(m_regs.Res_Y)/25.4f);
		iCounts = -iCounts;
		if (fDelta>0 && iCounts==0) // Enforce minimum motion of at least 1 count.
		{
			iCounts = -1;
		}
		else if (fDelta<0 && iCounts==0)
		{
			iCounts = 1;
		}
		m_fEPos = fEVal;
		m_fPPos = fPVal;
		m_fCurY = fEVal+fPVal;
		m_regs.DeltaXYHi = (iCounts >> 8) & 0b1111; //NOLINT
		m_regs.DYLow = iCounts & 0xFF; //NOLINT
		if (m_state != FS_JAM)
		{
			m_regs.MStatus = 0x80;
		}

}

uint8_t PAT9125::GetRegVal(uint8_t uiAddr)
{
	switch (uiAddr)
	{
		case 0x02:
		{
			uint8_t val = m_regs.MStatus;
			if (!m_bLoading)
			{
				m_regs.MStatus = 0; // clear motion flag.
			}
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
			//printf("Read DY: %d (%f) \n",m_regs.raw[uiAddr], (m_fYPos-m_fCurY));
			m_fYPos = m_fCurY;
		}
		/* FALLTHRU */
		default:
			//printf("Read: %02x, %02x\n",uiAddr, m_regs.raw[uiAddr]);
			return gsl::at(m_regs.raw,uiAddr);
	}
};

bool PAT9125::SetRegVal(uint8_t uiAddr, uint32_t uiData)
{
	if (!(m_uiRW  & (1u<<uiAddr)))
	{
		std::cerr << "PAT9125: tried to write Read-only register\n";
		return false; // RO register.
	}
	gsl::at(m_regs.raw,uiAddr) = gsl::narrow<uint8_t>(uiData);
	//printf("Wrote: %02x = %02x (%02x)\n",uiAddr,uiData, m_regs.raw[uiAddr]);
	return true;
};

IScriptable::LineStatus PAT9125::ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs)
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
			{
				return IssueLineError(std::string("Set value ") + std::to_string(iVal) + " is out of the range [0,3]" );
			}
			Set(static_cast<FSState>(iVal));
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
