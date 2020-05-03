/*
	uart_pty.h

	Copyright 2012 Michel Pollet <buserror@gmail.com>
	
	Rewritten 2020 to C++ by VintagePC <https://github.com/vintagepc/>

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


#ifndef __UART_PTY_H___
#define __UART_PTY_H___

#include <pthread.h>

#include <iostream>

#include "fifo_declare.h"

#include "BasePeripheral.h"

class uart_pty: public BasePeripheral
{

	public:
		#define IRQPAIRS _IRQ(BYTE_IN,"8<uart_pty.in") _IRQ(BYTE_OUT,"8>uart_pty.out")
		#include "IRQHelper.h"

		// Destructor. Kills the thread, if it was started.
		~uart_pty();

		void Init(avr_t *avr);

		void Connect(char chrUART);
		
		const std::string GetSlaveName() { return std::string(pty.slavename); };

	private:

		void* Run();
		void OnByteIn(avr_irq_t * irq, uint32_t value);
		void OnXOnIn(avr_irq_t * irq, uint32_t value);
		void OnXOffIn(avr_irq_t * irq, uint32_t value);
		avr_cycle_count_t OnFlushTimer(struct avr_t * avr,avr_cycle_count_t when);

		void FlushData();

		pthread_t	m_thread = 0;
		bool		m_bXOn = false;
		bool m_bQuit = false;


		DECLARE_FIFO(uint8_t,uart_pty_fifo, 512);

		DEFINE_FIFO(uint8_t,uart_pty_fifo);

		typedef struct uart_pty_port_t {
			unsigned int	tap : 1, crlf : 1;
			int 		s;			// socket we chat on
			char 		slavename[64];
			uart_pty_fifo_t in;
			uart_pty_fifo_t out;
			uint8_t		buffer[512];
			size_t		buffer_len, buffer_done;
		} uart_pty_port_t, *uart_pty_port_p;

		union {
			struct {
				uart_pty_port_t		pty;
				uart_pty_port_t		tap;
			};
			uart_pty_port_t port[2];
		};


};


#endif /* __UART_PTY_H___ */
