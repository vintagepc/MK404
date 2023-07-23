/*
	uart_pty.cpp

	Copyright 2012 Michel Pollet <buserror@gmail.com> as uart_pty.c

	Rewritten 2020 to C++ by VintagePC <https://github.com/vintagepc/>

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


#include "uart_pty.h"
#include "Config.h"
#include "avr_uart.h"                   // for AVR_IOCTL_UART_GETIRQ, ::AVR_...
#include "gsl-lite.hpp"
#include "sim_io.h"                     // for avr_io_getirq, avr_ioctl
#include "sim_time.h"                   // for avr_hz_to_cycles

#include <cerrno>                      // for errno
#include <cstdlib>                     // for getenv, atoi, system
#include <cstring>                     // for memset, strerror
#include <iostream>                      // for printf, NULL, fprintf, sprintf
#include <pthread.h>                    // for pthread_create, pthread_join
#if defined(__APPLE__)
// utils/Util.h clashes with this system file.
//#  include <util.h>                     // for openpty
// hack, prototype from `man openpty`
extern "C" {
int
    openpty(int *amaster, int *aslave, char *name, struct termios *termp, struct winsize *winp);
}
#else
# include <pty.h>                       // for openpty
#endif
#include <string>
#include <sys/select.h>                 // for select, FD_ISSET, FD_SET, FD_...
// IWYU pragma: no_include <bits/types/struct_timeval.h>
// IWYU pragma: no_include <bits/termios-struct.h>
// IWYU pragma: no_include <bits/termios-tcflow.h>
#include <sys/time.h>                   // IWYU pragma: keep
#include <termios.h>                    // IWYU pragma: keep // for cfmakeraw, tcgetattr, tcsetattr
#include <unistd.h>                     // for close, read, symlink, unlink

//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#else
#include "sim_hex.h"
#endif

using std::cout;
using std::cerr;
using std::stoi;

extern "C" {
	DEFINE_FIFO(uint8_t,uart_pty_fifo); //NOLINT - 3rd party file.
}

/*
 * called when a byte is send via the uart on the AVR
 */
void uart_pty::OnByteIn(struct avr_irq_t *, uint32_t value)
{
	TRACE(printf("uart_pty_in_hook %02x\n", value);)
	std::lock_guard<std::mutex> lock(m_lock);
	uart_pty_fifo_write(&pty.in, value);

	if (tap.s) {
		if (tap.crlf && value == '\n')
		{
			uart_pty_fifo_write(&tap.in, '\r');
		}
		uart_pty_fifo_write(&tap.in, value);
	}
}

// try to empty our fifo, the uart_pty_xoff_hook() will be called when
// other side is full
void uart_pty::FlushData()
{
	std::lock_guard<std::mutex> lock(m_lock);
	while (m_bXOn && !uart_pty_fifo_isempty(&pty.out)) {
		TRACE(int r = pty.out.read;)
		uint8_t byte = uart_pty_fifo_read(&pty.out);
		TRACE(printf("uart_pty_flush_incoming send r %03d:%02x\n", r, byte);)
		if (m_chrLast == '\n' && byte == '\n')
		{
			std::cout << "Swallowing repeated newlines\n";
		}
		else
		{
			if (byte !='\n')
			{
				m_chrLast = byte;
			}
			RaiseIRQ(BYTE_OUT, byte);

		}

		if (tap.s) {
			if (tap.crlf && byte == '\n')
			{
				uart_pty_fifo_write(&tap.in, '\r');
			}
			uart_pty_fifo_write(&tap.in, byte);
		}
	}
	if (tap.s) {
		while (m_bXOn && !uart_pty_fifo_isempty(&tap.out)) {
			uint8_t byte = uart_pty_fifo_read(&tap.out);
			if (tap.crlf && byte == '\r') {
				uart_pty_fifo_write(&tap.in, '\n');
			}
			if (byte == '\n')
			{
				continue;
			}
			uart_pty_fifo_write(&tap.in, byte);
			if (m_chrLast == '\n' && byte == '\n')
			{
				std::cout << "2Swallowing repeated newlines\n";
			}
			else
			{
				if (byte !='\n')
				{
					m_chrLast = byte;
				}
				RaiseIRQ(BYTE_OUT, byte);

			}
		}
	}
}

avr_cycle_count_t uart_pty::OnFlushTimer(struct avr_t *, avr_cycle_count_t when)
{
	FlushData();
	/* always return a cycle NUMBER not a cycle count */
	return m_bXOn ? when + avr_hz_to_cycles(m_pAVR, 1000) : 0;
}

/*
 * Called when the uart has room in it's input buffer. This is called repeateadly
 * if necessary, while the xoff is called only when the uart fifo is FULL
 */
void uart_pty::OnXOnIn(struct avr_irq_t * ,uint32_t)
{
	TRACE(if (!m_bXOn) printf("uart_pty_xon_hook\n");)
	m_bXOn = true;

	FlushData();

	// if the buffer is not flushed, try to do it later
	if (m_bXOn)	RegisterTimer(m_fcnFlush,avr_hz_to_cycles(m_pAVR, 1000),this);
}

/*
 * Called when the uart ran out of room in it's input buffer
 */
void uart_pty::OnXOffIn(struct avr_irq_t *, uint32_t)
{
	TRACE(if (m_bXOn) printf("uart_pty_xoff_hook\n");)
	m_bXOn = false;
	CancelTimer(m_fcnFlush,this);
}

uart_pty::uart_pty():pty({}),tap({})
{
	memset(&port[0], 0, sizeof(port[0]));
	memset(&port[1], 0, sizeof(port[1]));
}

void* uart_pty::Run()
{
	while (!m_bQuit) {
		fd_set read_set {}, write_set {};
		int max = 0;
		FD_ZERO(&read_set); //NOLINT - system library
		FD_ZERO(&write_set); //NOLINT

		for (auto &p : port)
		{
			if (p.s) {
				// read more only if buffer was flushed
				std::lock_guard<std::mutex> lock(m_lock);
				if (p.buffer_len == p.buffer_done) {
					FD_SET(p.s, &read_set); //NOLINT - system library
					max = p.s > max ? p.s : max;
				}
				if (!uart_pty_fifo_isempty(&p.in)) {
					FD_SET(p.s, &write_set); //NOLINT - system library
					max = p.s > max ? p.s : max;
				}
			}
		}

		// short, but not too short interval
		struct timeval timo = { 0, 500 };
		int ret = select(max+1, &read_set, &write_set, nullptr, &timo);

		if (ret < 0)
		{
			break;
		}

		for (auto &p : port)
		{
			if (p.s)
			{
				if (FD_ISSET(p.s, &read_set)) //NOLINT - system library
				{
					std::lock_guard<std::mutex> lock(m_lock);
					ssize_t r = read(p.s, p.buffer,
										sizeof(p.buffer)-1);
					p.buffer_len = r;
					p.buffer_done = 0;
					TRACE(if (!p.tap)
							hdump("pty recv", p.buffer, r);)
				}
				if (p.buffer_done < p.buffer_len)
				{
					// write them in fifo
					std::lock_guard<std::mutex> lock(m_lock);
					while (p.buffer_done < p.buffer_len &&
							!uart_pty_fifo_isfull(&p.out))
					{
						int index = p.buffer_done++;
						TRACE(int wi = p.out.write;)
						uart_pty_fifo_write(&p.out,
								gsl::at(p.buffer,index));
						TRACE(printf("w %3d:%02x (%d/%d) %s\n",
									wi, gsl::at(p.buffer,index),
									p.out.read,
									p.out.write,
									m_bXOn ? "XON" : "XOFF");)
					}
				}
				if (FD_ISSET(p.s, &write_set)) //NOLINT - system library
				{
					std::lock_guard<std::mutex> lock(m_lock);
					uint8_t _buffer[512];
					gsl::span<uint8_t> buffer {_buffer};
					// write them in fifo
					auto dst = buffer.begin();
					while (!uart_pty_fifo_isempty(&p.in) &&
							dst !=buffer.end())
					{
						*dst = uart_pty_fifo_read(&p.in);
						dst++;
					}
					size_t len = dst - buffer.begin();
					size_t r = write(p.s, buffer.data(), len);
					if (r!=len)
					{
						std::cerr << "Failed to write to PTY\n";
					}
					TRACE(if (!p.tap) hdump("pty send", buffer.data(), r);)
				}
			}
		}
		/* DO NOT call this, this create a concurency issue with the
		 * FIFO that can't be solved cleanly with a memory barrier
			uart_pty_flush_incoming(p);
		  */
	}
	return nullptr;
}

void uart_pty::InitPrivate()
{
	RegisterNotify(BYTE_IN, MAKE_C_CALLBACK(uart_pty,OnByteIn), this);

	int hastap = (getenv("SIMAVR_UART_TAP") && stoi(getenv("SIMAVR_UART_TAP"))) ||
			(getenv("SIMAVR_UART_XTERM") && stoi(getenv("SIMAVR_UART_XTERM"))) ;

	for (int ti = 0; ti < 1 + hastap; ti++) {
		int m, s;

		if (openpty(&m, &s, static_cast<char*>(gsl::at(port,ti).slavename), nullptr, nullptr) < 0) {
			std::cerr <<  "uart_pty::Init: Can't create pty: " << strerror(errno);
			return ;
		}
		struct termios tio {};
		tcgetattr(m, &tio);
		cfmakeraw(&tio);
		tcsetattr(m, TCSANOW, &tio);
		gsl::at(port,ti).s = m;
		gsl::at(port,ti).tap = ti != 0;
		gsl::at(port,ti).crlf = ti != 0;
		std::cout << "uart_pty_init " << (ti == 0 ? "bridge" : "tap") << " on port *** " << static_cast<char*>(gsl::at(port,ti).slavename) << " ***\n";
	}

	auto fRunCB =[](void * param) { auto p = static_cast<uart_pty*>(param); return p->Run();};
	pthread_create(&m_thread, nullptr, fRunCB, this);

}

void uart_pty::BypassXON()
{
	OnXOnIn(nullptr, 0);
}

void uart_pty::Init(struct avr_t * avr)
{
	_Init(avr,this);
	InitPrivate();
	BypassXON();
}

void uart_pty::Init(struct avr_t * avr, char uart)
{
	_Init(avr,this);
	uint32_t f = 0;
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_GET_FLAGS(uart), &f); //NOLINT - complaint in external macro
	f &= ~AVR_UART_FLAG_POLL_SLEEP; // Issue #356
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_SET_FLAGS(uart), &f); //NOLINT - complaint in external macro

	InitPrivate();

}

// Shuts down the thread on destruction.
uart_pty::~uart_pty()
{
	if (!m_thread)
	{
		return;
	}
	std::cout << static_cast<const char*>(__func__) << '\n';
	m_bQuit = true; // Signal thread it's time to quit.
	for (auto &p: port)
	{
		if (p.s)
		{
			close(p.s);
		}
	}
	pthread_join(m_thread, nullptr);
}

void uart_pty::Connect(char uart)
{
	// disable the stdio dump, as we are sending binary there
	uint32_t f = 0;
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_GET_FLAGS(uart), &f); //NOLINT - complaint in external macro
	f &= ~AVR_UART_FLAG_STDIO;
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_SET_FLAGS(uart), &f); //NOLINT - complaint in external macro

	avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(uart), UART_IRQ_OUTPUT); //NOLINT - complaint in external macro
	avr_irq_t * dst = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(uart), UART_IRQ_INPUT); //NOLINT - complaint in external macro
	avr_irq_t * xon = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(uart), UART_IRQ_OUT_XON); //NOLINT - complaint in external macro
	avr_irq_t * xoff = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(uart), UART_IRQ_OUT_XOFF); //NOLINT - complaint in external macro
	if (src && dst)
	{
		ConnectFrom(src, BYTE_IN);
		ConnectTo(BYTE_OUT, dst);
	}
	if (xon) avr_irq_register_notify(xon, MAKE_C_CALLBACK(uart_pty,OnXOnIn), this);
	if (xoff) avr_irq_register_notify(xoff, MAKE_C_CALLBACK(uart_pty,OnXOffIn),this);

	if (port[0].s) {
		std::string strLnk("/tmp/simavr-uart");
		// if (ti==1)
		// {
		// 	strLnk +="tap";
		// }
		strLnk+=uart;
		unlink(strLnk.c_str());
		if (symlink(static_cast<char*>(port[0].slavename), strLnk.c_str()) != 0)
		{
			std::cerr << "WARN: Can't create " << strLnk << " " << strerror(errno);
		}
		else
		{
			std::cout << strLnk << " now points to: " << static_cast<char*>(port[0].slavename) << '\n';
		}
	}
	if (getenv("SIMAVR_UART_XTERM") && stoi(getenv("SIMAVR_UART_XTERM")))
	{
		std::string strCmd("xterm -e picocom -b 115200 ");
		strCmd += static_cast<char*>(tap.slavename);
		strCmd += " >/dev/null 2>&1 &";
		if (system(strCmd.c_str())<0) //NOLINT - no user-alterable params inside...
		{
			std::cout << "Could not launch xterm\n";
		}
	}
	else
	{
		std::cout << "note: export SIMAVR_UART_XTERM=1 and install picocom to get a terminal\n";
	}
}

void uart_pty::ConnectPTYOnly(const std::string& strLnk)
{
	if (port[0].s) {
		unlink(strLnk.c_str());
		if (symlink(static_cast<char*>(port[0].slavename), strLnk.c_str()) != 0)
		{
			std::cerr << "WARN: Can't create " << strLnk << " " << strerror(errno);
		}
		else
		{
			std::cout << strLnk << " now points to: " << static_cast<char*>(port[0].slavename) << '\n';
		}
	}
}
