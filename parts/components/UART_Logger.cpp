/*
	UART_Logger.cpp - Logs a UART to console and file.

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

#include "UART_Logger.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "avr_uart.h"
#include <fcntl.h>


//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif


void UART_Logger::OnByteIn(struct avr_irq_t * irq, uint32_t value)
{
    uint8_t c = value;
    write(m_fdOut,&c,1);
    // and terminal
    printf("UART%c: 0x%02x\n",m_chrUART,c);
}

void UART_Logger::Init(struct avr_t * avr, char chrUART)
{
	_Init(avr, this);
	m_chrUART = chrUART;
	RegisterNotify(BYTE_IN, MAKE_C_CALLBACK(UART_Logger, OnByteIn),this);   
		// disable the stdio dump, as we're pritning in hex. 
	uint32_t f = 0;
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_GET_FLAGS(chrUART), &f);
	f &= ~AVR_UART_FLAG_STDIO;
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_SET_FLAGS(chrUART), &f);

	avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUTPUT);
	if (src) 
		ConnectFrom(src, BYTE_IN);
	
    m_strFile[8] = chrUART;

    // open the file
	m_fdOut = open(m_strFile.c_str(), O_RDWR|O_CREAT, 0644);
	if (m_fdOut < 0) {
		perror(m_strFile.c_str());
	}
	// Truncate the file (start new)
	(void)ftruncate(m_fdOut, 0);

    printf("UART %c is now logging to %s\n",m_chrUART,m_strFile.c_str());

}

UART_Logger::~UART_Logger()
{
	close(m_fdOut);
	printf("UART logger finished.\n");
}

