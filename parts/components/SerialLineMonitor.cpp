/*
	GCodeSniffer.cpp - utility for sniffing GCODE from a serial connection.
	Its main purpose is for sniffing T-codes from the MMU. But it might have use elsewhere...

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


#include "SerialLineMonitor.h"
#include "avr_uart.h"  // for AVR_IOCTL_UART_GETIRQ, ::UART_IRQ_INPUT, ::UAR...
#include "sim_io.h"    // for avr_io_getirq
#include <algorithm>         // for copy
#include <iostream>    // for operator<<, basic_ostream, cout, ostream


SerialLineMonitor::SerialLineMonitor(const std::string &strName):Scriptable(strName)
{
	RegisterAction("WaitForLine","Waits for the provided line to appear on the serial output.",WaitForLine, {ArgType::String});
	RegisterAction("WaitForLineContains","Waits for the serial output to contain a line with the given string.",WaitForContains,{ArgType::String});
	RegisterAction("SendGCode","Sends the specified string as G-Code.",SendGCode,{ArgType::String});
	RegisterAction("NextLineMustBe","Errors if the next output line is not as specified.",NextLineMustBe, {ArgType::String});
	m_strLine.reserve(100);
};

void SerialLineMonitor::OnByteIn(struct avr_irq_t *, uint32_t value)
{
    unsigned char c = value&0xFFU;
	bool bNewLine = (c == 0x0a);

	if (!bNewLine)
	{
		m_strLine.push_back(c);
	}
	else
	{
		OnNewLine();
	}
}

void SerialLineMonitor::OnXOnIn(struct avr_irq_t *, uint32_t)
{
	m_bXOn = true;
}

void SerialLineMonitor::OnXOffIn(struct avr_irq_t *, uint32_t)
{
	m_bXOn = false;
}


Scriptable::LineStatus SerialLineMonitor::ProcessAction(unsigned int ID, const std::vector<std::string> &args)
{
	if (m_type != None && m_strMatch == args.at(0)) // already in wait state for same find
	{
		if (m_iLineCt>0 && !m_bMatched && ID == NextLineMustBe) // Failed to match on the next line.
		{
			m_strMatch.clear();
			std::cout << "NextLine Mismatch, received: " << m_strLine << '\n';
			m_type = None;
			m_bMatched = false;
			return LineStatus::Timeout;
		}
		else if (!m_bMatched)
		{
			return LineStatus::Waiting;
		}
		m_strMatch.clear();
		m_type = None;
		m_bMatched = false;
		return LineStatus::Finished;
	}

	switch(ID)
	{
		case WaitForLine:
		case WaitForContains:
			m_bMatched = false;
			m_strMatch = args[0];
			m_type = (ID == WaitForLine) ? Full : Contains;
			return LineStatus::Waiting;
		case NextLineMustBe:
			m_iLineCt = 0;
			m_bMatched = false;
			m_strMatch = args[0];
			m_type = MustBe;
			return LineStatus::Waiting;
		case SendGCode:
			if (m_strGCode.empty())
			{
				m_strGCode = args.at(0)+ '\n';
				m_itGCode = m_strGCode.begin();
			}
			return SendChar();

	}
	return LineStatus::Unhandled;
}

Scriptable::LineStatus SerialLineMonitor::SendChar()
{
	if (!m_bXOn)
	{
		return LineStatus::Waiting;
	}
	RaiseIRQ(BYTE_OUT,m_itGCode[0]);
	m_itGCode++;
	if (m_itGCode!=m_strGCode.end())
	{
		return LineStatus::Waiting;
	}
	else
	{
		m_strGCode.clear();
		return LineStatus::Finished;
	}

	return LineStatus::Unhandled;
}

void SerialLineMonitor::OnNewLine()
{
	if(!m_type)
	{
		m_strLine.clear();
		return; // No match configured.
	}
	m_iLineCt++;
	switch (m_type)
	{
		case Full:
		case MustBe:
			m_bMatched = (m_strLine == m_strMatch);
			break;
		case Contains:
			m_bMatched = m_strLine.find(m_strMatch) != std::string::npos;
			break;
		case None:
			break;
	}
	m_strLine.clear();
}

void SerialLineMonitor::Init(struct avr_t * avr, char chrUART)
{
	_Init(avr, this);
	m_chrUART = chrUART;
	RegisterNotify(BYTE_IN, MAKE_C_CALLBACK(SerialLineMonitor, OnByteIn),this);

	avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUTPUT); //NOLINT - complaint in external macro
	avr_irq_t * dst = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_INPUT); //NOLINT - complaint in external macro
	avr_irq_t * xon = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUT_XON); //NOLINT - complaint in external macro
	avr_irq_t * xoff = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUT_XOFF); //NOLINT - complaint in external macro
	if (src && dst) {
		ConnectFrom(src, BYTE_IN);
		ConnectTo(BYTE_OUT, dst);
	}
	if (xon) avr_irq_register_notify(xon, MAKE_C_CALLBACK(SerialLineMonitor,OnXOnIn), this);
	if (xoff) avr_irq_register_notify(xoff, MAKE_C_CALLBACK(SerialLineMonitor,OnXOffIn),this);
}
