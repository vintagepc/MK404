/*
	SPIPeripheral.cpp - Generalization helper for SPI-based peripherals.
    This header auto-wires the SPI and deals with some of the copypasta
    relating to checking CSEL and so on. You just need to have
    OnSPIIn and (optionally) OnCSELIn overriden, as well as the
    SPI_BYTE_[*]/SPI_CSEL IRQs defined.

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

#include "SPIPeripheral.h"
#include "avr_spi.h"
#include "sim_io.h"   // for avr_io_getirq

// Sets up the IRQs on "avr" for this class
void SPIPeripheral::OnPostInit(avr_t* avr, unsigned int eCSEL)
{
	avr_irq_register_notify(avr_io_getirq(avr,AVR_IOCTL_SPI_GETIRQ(0),SPI_IRQ_OUTPUT), MAKE_C_CALLBACK(SPIPeripheral,_OnSPIIn), this); //NOLINT - complaint in external macro

	m_pSPIReply = avr_io_getirq(avr,AVR_IOCTL_SPI_GETIRQ(0), SPI_IRQ_INPUT); //NOLINT - complaint in external macro

	RegisterNotify(eCSEL, MAKE_C_CALLBACK(SPIPeripheral,_OnCSELIn), this);
}

void SPIPeripheral::_OnCSELIn(struct avr_irq_t * irq, uint32_t value)
{
	m_bCSel = value;
	OnCSELIn(irq,value);
};

void SPIPeripheral::_OnSPIIn(struct avr_irq_t * irq, uint32_t value)
{
	if (!m_bCSel)
	{
		m_bSendReply = false;
		uint8_t uiByteOut = OnSPIIn(irq, value);
		if (m_bSendReply)
		{
			avr_raise_irq(m_pSPIReply,uiByteOut);
		}
	}
}
