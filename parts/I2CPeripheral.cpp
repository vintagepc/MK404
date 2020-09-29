/*
	I2CPeripheral.cpp - Generalization helper for I2C-based peripherals.
    This header auto-wires the bus and deals with some of the copypasta.
    Currently only supports bit-banged mode as I haven't needed to deal
	with "real" i2c hardware yet.

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


#include "I2CPeripheral.h"
#include <cstdint>            // for uint8_t, uint32_t, int32_t, uint16_t

I2CPeripheral::I2CPeripheral(uint8_t uiAddress):m_uiDevAddr(uiAddress)
{

}

bool I2CPeripheral::ProcessByte(const uint8_t &value)
{
	bool bReturn = false;
	switch (m_state)
	{
		case State::AddrIn:
		{
			msgIn.bytes[2] = value;
			if (msgIn.address != m_uiDevAddr)
			{
				m_SCLState = SCLS::Idle;
				m_state = State::Idle;
				break;
			}
			bReturn = true;
			if (msgIn.isAddrRead)
			{
				m_SCLState = SCLS::WaitForWrite; // Wait for next clock
			}
			else
			{
				m_state = State::RegIn;
			}
		}
		break;
		case State::RegIn:
		{
			msgIn.bytes[1] = value;
			m_state = State::DataIn;
			bReturn = true;
			// NOTE - this doesn't handle OOB regs.
		}
		break;
		case State::DataIn:
		{
			msgIn.bytes[0] = value;
			bReturn = SetRegVal(msgIn.writeRegAddr++,msgIn.data);
		}
		break;
		default:
		break;
	}
	//printf("I2C msg status: Addr: %02x Read: %u raw: %04x\n", msgIn.address, msgIn.isAddrRead, msgIn.raw);
	return bReturn;
}

void I2CPeripheral::_OnSCL(avr_irq_t *irq, uint32_t value)
{
	//if (value) printf("SCL: %u (%u)\n", value,m_uiBitCt);
	bool bClockRise = !(irq->value) && value;
	bool bClockFall = (irq->value) && !value;
	if  (m_SCLState ==SCLS::Idle || (bClockRise==bClockFall))
	{
		return;
	}
	switch (m_SCLState)
	{
		default:
			break;
		case SCLS::Reading:
			if (bClockRise)
			{
				if (m_uiBitCt == 8) // Byte done, do we ACK or NACK?
				{
					if (ProcessByte(m_uiByte))
					{
						//printf("ACK\n");
						avr_raise_irq(m_pSDA,0);
					}
					// else
					// 	printf("NACK\n");
					m_uiByte = m_uiBitCt = 0;
				}
				else
				{
					m_uiByte <<= 1; // Shift up.
					m_uiByte |= (m_pSDA->value & 1u);
					m_uiBitCt++;
				}
			}
			break;
		case SCLS::WaitForWrite:
			if (bClockFall)
			{
				m_SCLState = SCLS::Writing;
			}
			break;
		case SCLS::Writing:
			{
				if (bClockRise)
				{
					if (m_uiBitCt == 0)
					{
						m_uiBitCt = 8;
						m_uiByte = GetRegVal(msgIn.writeRegAddr);
						//printf("Sending %02x\n",m_uiByte);
					}
					avr_raise_irq(m_pSDA, static_cast<unsigned>(m_uiByte)>>(--m_uiBitCt) &1u);
				}
				else
				{
					if (m_uiBitCt == 0)
					{
						m_SCLState = SCLS::WaitForACK;
					}
				}
			}
			break;
		case SCLS::WaitForACK:
			{
				if (bClockRise)
				{
					//printf("%cACK rec'd\n",m_pSDA->value? ' ':'N');
				}
				else
				{
					m_SCLState = SCLS::Writing; // Back to writing.
				}
			}
			break;
	}
}

void I2CPeripheral::_OnSDA(avr_irq_t */*irq*/, uint32_t value)
{
	if (m_pSCL->value)
	{
		if (value)
		{
			m_SCLState = SCLS::Idle;
		}
		else
		{
			m_SCLState = SCLS::Reading;
			m_state = State::AddrIn;
			m_uiBitCt = 0;
			m_uiByte = 0;
		}
		//printf("I2C TX %s\n",value?"Stop" : "Start");
	}
}
