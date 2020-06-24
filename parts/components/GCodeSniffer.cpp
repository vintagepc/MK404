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

#include "GCodeSniffer.h"
#include <stdio.h>     // for printf
#include "avr_uart.h"  // for ::UART_IRQ_OUTPUT, AVR_IOCTL_UART_GETIRQ
#include "sim_io.h"    // for avr_io_getirq

void GCodeSniffer::OnByteIn(struct avr_irq_t * irq, uint32_t value)
{
    unsigned char c = value&0xFF;
	if (m_bNewLine && c==m_chrCode)
	{
		m_bCapture = true;
		return;
	}

	m_bNewLine = (c == 0x0a);

	if (m_bCapture)
	{
		if (c == ' ' || m_bNewLine)
		{
			m_bCapture = false;
			printf("Captured code %s\n", m_strLine.c_str());
			uint32_t uiOut = stoi(m_strLine);
			RaiseIRQ(CODEVAL_OUT,uiOut);
			m_strLine.clear();
		}
		else
			m_strLine.push_back(c);
	}
}

void GCodeSniffer::Init(struct avr_t * avr, char chrUART)
{
	_Init(avr, this);
	m_chrUART = chrUART;
	RegisterNotify(BYTE_IN, MAKE_C_CALLBACK(GCodeSniffer, OnByteIn),this);
		// disable the stdio dump, as we're pritning in hex.

	avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUTPUT);
	if (src)
		ConnectFrom(src, BYTE_IN);

    printf("UART %c is now being monitored for %c codes\n",m_chrUART,m_chrCode);

}
