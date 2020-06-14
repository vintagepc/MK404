/*
	GCodeSniffer.cpp - utility for sniffing GCODE from a serial connection.
	Its main purpose is for sniffing T-codes from the MMU. But it might have use elsewhere...

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

#include "SerialLineMonitor.h"
#include <stdio.h>
#include "avr_uart.h"



void SerialLineMonitor::OnByteIn(struct avr_irq_t * irq, uint32_t value)
{
    unsigned char c = value&0xFF;
	bool bNewLine = (c == 0x0a);

	if (!bNewLine)
		m_strLine.push_back(c);
	else
		OnNewLine();
}

Scriptable::LineStatus SerialLineMonitor::ProcessAction(unsigned int ID, const vector<string> &args)
{
	if (m_type != None) // already in wait state
	{
		if (!m_bMatched)
			return LineStatus::Waiting;
		m_strMatch.clear();
		m_type = None;
		m_bMatched = false;
		return LineStatus::Finished;
	}

	m_bMatched = false;
	m_strMatch = args[0];
	switch(ID)
	{
		case WaitForLine:
			m_type = Full;
			break;
		case WaitForContains:
			m_type = Contains;
			break;
	}
	return LineStatus::Waiting;
}

void SerialLineMonitor::OnNewLine()
{
	if(!m_type)
		return; // No match configured.
	switch (m_type)
	{
		case Full:
			m_bMatched = m_strLine.compare(m_strMatch)==0;
			break;
		case Contains:
			m_bMatched = m_strLine.find(m_strMatch) != string::npos;
			break;
	}
}

void SerialLineMonitor::Init(struct avr_t * avr, char chrUART)
{
	_Init(avr, this);
	m_chrUART = chrUART;
	RegisterNotify(BYTE_IN, MAKE_C_CALLBACK(SerialLineMonitor, OnByteIn),this);
		// disable the stdio dump, as we're pritning in hex.

	avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUTPUT);
	if (src)
		ConnectFrom(src, BYTE_IN);
}
