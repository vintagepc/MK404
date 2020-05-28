/*
	HC595.h - a shift register for the MMU.

	Original SPI portion Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

	Rewritten for C++ and a bitbanged connection in 2020, VintagePC <https://github.com/vintagepc/>

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

#include "HC595.h"

/*
 * called when a SPI byte is sent
 */
void HC595::OnSPIIn(struct avr_irq_t * irq, uint32_t value)
{
	// send "old value" to any chained one..
	RaiseIRQ(SPI_BYTE_OUT,m_uiValue);
	m_uiValue = m_uiValue<<8 | (value & 0xFF);
}

void HC595::OnDataIn(struct avr_irq_t * irq, uint32_t value)
{
	m_uiCurBit = value&1;
}

void HC595::OnClockIn(struct avr_irq_t * irq, uint32_t value)
{
	if (irq->value && !value)
		m_uiValue = m_uiValue<<1 | (m_uiCurBit);	
}

/*
 * called when a LATCH signal is sent
 */
void HC595::OnLatchIn(struct avr_irq_t * irq, uint32_t value)
{
	if (!irq->value && value) {	// rising edge
		uint32_t uiChanged = m_uiLatch ^ m_uiValue; // Grab the bits that have changed since last latch.
		m_uiLatch = m_uiValue;
		RaiseIRQ(OUT, m_uiLatch);
		for (int i=0; i<32; i++)
			if (uiChanged & (1<<i))
				RaiseIRQ(BIT0+i,(m_uiLatch>>i) & 1);

	}
}

/*
 * called when a RESET signal is sent
 */
void HC595::OnResetIn(struct avr_irq_t * irq, uint32_t value)
{
	if (irq->value && !value) 	// falling edge
		m_uiLatch = m_uiValue = 0;
}

void HC595::Init(struct avr_t * avr)
{
	_Init(avr, this);
	RegisterNotify(SPI_BYTE_IN, MAKE_C_CALLBACK(HC595,OnSPIIn), 	this);
	RegisterNotify(IN_LATCH, 	MAKE_C_CALLBACK(HC595,OnLatchIn), 	this);
	RegisterNotify(IN_RESET, 	MAKE_C_CALLBACK(HC595,OnResetIn), 	this);
	RegisterNotify(IN_CLOCK, 	MAKE_C_CALLBACK(HC595,OnClockIn), 	this);
	RegisterNotify(IN_DATA, 	MAKE_C_CALLBACK(HC595,OnDataIn), 	this);
}

