/*
	UART_Logger.cpp - Logs a UART to console and file.

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

#include "UART_Logger.h"
#include "avr_uart.h"  // for ::AVR_UART_FLAG_STDIO, ::UART_IRQ_OUTPUT, AVR_...
#include "sim_io.h"    // for avr_ioctl, avr_io_getirq
#include <iostream>     // for printf, perror


//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif

using std::cerr;
using std::cout;

void UART_Logger::OnByteIn(struct avr_irq_t *, uint32_t value)
{
	if (!m_fsOut.is_open())
	{
		return;
	}
    uint8_t c = value;
    m_fsOut.put(c);
	if (!m_fsOut.fail())
	{
	    std::cout << "UART" << m_chrUART << ": " << std::hex << c << '\n';
	}
	else
	{
		std::cerr << "UART Logger: failed to write to file\n";
	}
}

void UART_Logger::Init(struct avr_t * avr, char chrUART)
{
	_Init(avr, this);
	m_chrUART = chrUART;
	RegisterNotify(BYTE_IN, MAKE_C_CALLBACK(UART_Logger, OnByteIn),this);
		// disable the stdio dump, as we're pritning in hex.
	uint32_t f = 0;
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_GET_FLAGS(chrUART), &f); //NOLINT - complaint is external macro
	f &= ~AVR_UART_FLAG_STDIO;
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_SET_FLAGS(chrUART), &f); //NOLINT - complaint is external macro

	avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUTPUT); //NOLINT - complaint is external macro
	if (src) ConnectFrom(src, BYTE_IN);

    m_strFile[8] = chrUART;

    // open the file
	m_fsOut.open(m_strFile, m_fsOut.binary | m_fsOut.out | m_fsOut.trunc);
	if (!m_fsOut.is_open())
	{
		std::cerr << "Failed to open output file for UART_Logger\n";
	}
	else
	{
    	std::cout << "UART " << m_chrUART << " is now logging to " << m_strFile << '\n';
	}
}

UART_Logger::~UART_Logger()
{
	m_fsOut.close();
	std::cout << "UART logger finished.\n";
}
