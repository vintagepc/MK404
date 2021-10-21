/*
	MCP23S17.cpp - simulated GPIO extender.

	Copyright 2021 VintagePC <https://github.com/vintagepc/>

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

#include "MCP23S17.h"
#include "TelemetryHost.h"
#include "gsl-lite.hpp"
#include <cstring>           // for memset

enum {
	R_OFF_IODIR,
	R_OFF_IPOL,
	R_OFF_GPINTEN,
	R_OFF_DEFVAL,
	R_OFF_INTCON,
	R_OFF_IOCON,
	R_OFF_GPPU,
	R_OFF_INTF,
	R_OFF_INTCAP,
	R_OFF_GPIO,
	R_OFF_OLAT,
	R_OFF_END
};

/*
 * called when a SPI byte is received. It's guarded by the SPIPeripheral CSEL check.
 */
uint8_t MCP23S17::OnSPIIn(struct avr_irq_t *, uint32_t value)
{
	// Just for tracing, the bytes are not attached to anything.
	RaiseIRQ(SPI_BYTE_IN, value);
    // (printf("GPIO: byte received: %02x\n",value));
    //TRACE(printf("TMC2130 %c: Clocking (%10lx) out %02x\n",m_cAxis.load(),m_cmdOut.all,byte));
	uint8_t byte = 0xFF;
	switch (m_state) {
		case MCP_STATE::IDLE:
			m_hdr.opcode = value & 0xFFU;
			m_state = MCP_STATE::ADDR;
			break;
		case MCP_STATE::ADDR:
			m_addr = value & 0xFFU;
			m_state = MCP_STATE::DATA;
			break;
		case MCP_STATE::DATA:
			if (m_hdr.READ)
			{
				byte = m_regs.raw[ToBank1Addr(m_addr)];
				if (ToBank1Addr(m_addr) >= 0x12)
				{
					// printf("MCP: READ %02x fr %02x\n", byte, ToBank1Addr(m_addr));
				}
			}
			else
			{
				OnWrite(ToBank1Addr(m_addr),value);
			}
			if (m_regs.bank[0].IOCON.SEQOP==0)
			{
				m_addr++;
			}
		break;
		default:
			break;

	}
    SetSendReplyFlag();
	RaiseIRQ(SPI_BYTE_OUT,byte);
    return byte; // SPIPeripheral takes care of the reply.
}

void MCP23S17::OnODRChanged(uint8_t bank, uint8_t old, uint8_t value)
{
	auto uiChanged = old^value;
	auto base = bank ? MCP_GPB0 : MCP_GPA0;
	auto uiDir = ~(m_regs.bank[bank].IODIR);
	uint8_t uiPos = 0;
	while (uiChanged)
	{
		// Pin must be in output mode to fire IRQ.
		if (uiChanged & 1U & uiDir)
		{
			m_bSelf = true;
			RaiseIRQ(base + uiPos, value & 1U);
			m_bSelf = false;
		}
		uiChanged >>= 1U;
		value >>= 1U;
		uiDir >>= 1U;
		uiPos++;
	}
}


void MCP23S17::OnWrite(uint8_t b1_addr, uint8_t value)
{
	bool bank = b1_addr >= R_OFF_END;
	switch (b1_addr % 0x10)
	{
		case R_OFF_GPIO:
		case R_OFF_OLAT:
		{
			OnODRChanged(bank, m_regs.bank[bank].OLAT, value);
			auto inMask = (m_regs.bank[bank].IODIR);
			value = value & ~inMask;
			value |= (m_regs.bank[bank].OLAT & inMask);
			m_regs.bank[bank].OLAT = value;
			//printf("ODR update: %02x %02x / I: %02x %02x\n", m_regs.bank[0].OLAT, m_regs.bank[1].OLAT, m_regs.bank[0].GPIO,m_regs.bank[1].GPIO);
		}
			break;
		default:
			//printf("MCP: Wrote %02x to %02x\n", value, ToBank1Addr(m_addr));
			m_regs.raw[b1_addr] = value;
	}
}

void MCP23S17::OnPinChanged(struct avr_irq_t * irq,uint32_t value)
{
	if (m_bSelf)
	{
		// Reentrancy guard
		return;
	}
	uint8_t pin = 0U;
	uint8_t bank = 0U;
	if (irq->irq >= MCP_GPB0)
	{
		pin = irq->irq - MCP_GPB0;
		bank = 1U;
	}
	else
	{
		pin = irq->irq - MCP_GPA0;
	}
	auto pin_mask = 1U << pin;
	//only update GPIO if the dir is in.
	if (m_regs.bank[bank].IODIR & pin_mask)
	{
		//printf("MCP: Pin changed: %c %02x to %d (was %02x)", 'A' + bank, pin, value,m_regs.bank[bank].GPIO);
		if (value)
		{
			m_regs.bank[bank].GPIO |= pin_mask;
		}
		else
		{
			m_regs.bank[bank].GPIO &= ~pin_mask;
		}
		//printf(" now %02x\n",m_regs.bank[bank].GPIO);
	}
}

// Converts the SPI address to a bank1-style (non-interleaved) addr.
uint8_t MCP23S17::ToBank1Addr(uint8_t addr)
{
	// internal config is the same as BANK=1.
	if (m_regs.bank[0].IOCON.BANK == 0)
	{
		uint8_t bank = addr & 0b1U;
		addr >>= 1U;
		addr |= (bank<<4U);
	}
	return addr;
}

// Called when CSEL changes.
void MCP23S17::OnCSELIn(struct avr_irq_t *, uint32_t value)
{
	if (value)
	{
		m_state = MCP_STATE::IDLE;
	}
}

// needed because cppcheck doesn't seem to do bitfield unions correctly.
// cppcheck-suppress uninitMemberVar
MCP23S17::MCP23S17()
{
	// Check register packing/sizes:
	static_assert(sizeof(m_regs.bank[0].GPIO)==sizeof(uint8_t));
	static_assert(sizeof(m_regs.bank)==sizeof(m_regs));
	static_assert(sizeof(m_regs.bank)==sizeof(m_regs.raw));
	static_assert(sizeof(m_regs.bank[0])==0x10);

}

void MCP23S17::Init(struct avr_t * avr)
{
    _InitWithArgs(avr, this, nullptr, SPI_CSEL);

	for (int i = MCP_GPA0; i <= MCP_GPB7; i++)
	{
		RegisterNotify(i, MAKE_C_CALLBACK(MCP23S17,OnPinChanged), this);
	}

	memset(&m_regs.raw,0,sizeof(m_regs.raw));
	m_regs.bank[0].IODIR = 0xFF;
	m_regs.bank[0].GPIO |= 0b110;
	m_regs.bank[1].IODIR = 0xFF;

	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, SPI_BYTE_IN,{TC::SPI, TC::Stepper},8);
	TH.AddTrace(this, SPI_BYTE_OUT,{TC::SPI, TC::Stepper},8);
	TH.AddTrace(this, SPI_CSEL, {TC::SPI, TC::Stepper, TC::OutputPin});
}
