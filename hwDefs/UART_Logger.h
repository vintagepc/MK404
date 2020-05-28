/*
	UART_Logger.h - Logs a UART to console and file.

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

#ifndef __UART_LOGGER_H___
#define __UART_LOGGER_H___

#include "BasePeripheral.h"
#include <iostream>

using namespace std;

class UART_Logger : public BasePeripheral
{
	public:
		#define IRQPAIRS _IRQ(BYTE_IN,"8<logger.in")
		#include "IRQHelper.h"

		~UART_Logger();

		void Init(avr_t *avr, char chrUART);

	private:

		void OnByteIn(avr_irq_t *irq, uint32_t value);

		string m_strFile = "LOG_UARTx.bin";
		char m_chrUART = '0';
		int m_fdOut = 0; // File handle.
};

#endif /* __UART_LOGGER_H___ */
