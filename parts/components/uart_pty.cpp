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
#include <sys/time.h>                   // IWYU pragma: keep
#include <errno.h>                      // for errno
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
#include <stdio.h>                      // for printf, NULL, fprintf, sprintf
#include <stdlib.h>                     // for getenv, atoi, system
#include <string.h>                     // for memset, strerror
#include <sys/select.h>                 // for select, FD_ISSET, FD_SET, FD_...
#include <termios.h>                    // for cfmakeraw, tcgetattr, tcsetattr
#include <unistd.h>                     // for close, read, symlink, unlink
#include "avr_uart.h"                   // for AVR_IOCTL_UART_GETIRQ, ::AVR_...
#include "sim_io.h"                     // for avr_io_getirq, avr_ioctl
#include "sim_time.h"                   // for avr_hz_to_cycles

//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif

extern "C" {
DEFINE_FIFO(uint8_t,uart_pty_fifo);
}

/*
 * called when a byte is send via the uart on the AVR
 */
void uart_pty::OnByteIn(struct avr_irq_t * irq, uint32_t value)
{
	TRACE(printf("uart_pty_in_hook %02x\n", value);)
	std::lock_guard<std::mutex> lock(m_lock);
	uart_pty_fifo_write(&pty.in, value);

	if (tap.s) {
		if (tap.crlf && value == '\n')
			uart_pty_fifo_write(&tap.in, '\r');
		uart_pty_fifo_write(&tap.in, value);
	}
}

// try to empty our fifo, the uart_pty_xoff_hook() will be called when
// other side is full
void uart_pty::FlushData()
{
	std::lock_guard<std::mutex> lock(m_lock);
	while (m_bXOn && !uart_pty_fifo_isempty(&pty.out)) {
		TRACE(int r = p->pty.out.read;)
		uint8_t byte = uart_pty_fifo_read(&pty.out);
		TRACE(printf("uart_pty_flush_incoming send r %03d:%02x\n", r, byte);)
		if (m_chrLast == '\n' && byte == '\n')
			printf("Swallowing repeated newlines\n");
		else
		{
			if (byte !='\n')
				m_chrLast = byte;
			RaiseIRQ(BYTE_OUT, byte);

		}

		if (tap.s) {
			if (tap.crlf && byte == '\n')
				uart_pty_fifo_write(&tap.in, '\r');
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
				continue;
			uart_pty_fifo_write(&tap.in, byte);
			if (m_chrLast == '\n' && byte == '\n')
				printf("2Swallowing repeated newlines\n");
			else
			{
				if (byte !='\n')
					m_chrLast = byte;
				RaiseIRQ(BYTE_OUT, byte);

			}
			}
	}
}

avr_cycle_count_t uart_pty::OnFlushTimer(struct avr_t * avr, avr_cycle_count_t when)
{
	FlushData();
	/* always return a cycle NUMBER not a cycle count */
	return m_bXOn ? when + avr_hz_to_cycles(m_pAVR, 1000) : 0;
}

/*
 * Called when the uart has room in it's input buffer. This is called repeateadly
 * if necessary, while the xoff is called only when the uart fifo is FULL
 */
void uart_pty::OnXOnIn(struct avr_irq_t * irq,uint32_t value)
{
	TRACE(if (!m_bXOn) printf("uart_pty_xon_hook\n");)
	m_bXOn = true;

	FlushData();

	// if the buffer is not flushed, try to do it later
	if (m_bXOn)
			RegisterTimer(m_fcnFlush,avr_hz_to_cycles(m_pAVR, 1000),this);
}

/*
 * Called when the uart ran out of room in it's input buffer
 */
void uart_pty::OnXOffIn(struct avr_irq_t * irq, uint32_t value)
{
	TRACE(if (m_bXOn) printf("uart_pty_xoff_hook\n");)
	m_bXOn = false;
	CancelTimer(m_fcnFlush,this);
}

uart_pty::uart_pty()
{
	memset(&port[0], 0, sizeof(port[0]));
	memset(&port[1], 0, sizeof(port[1]));
}

void* uart_pty::Run()
{
	while (!m_bQuit) {
		fd_set read_set, write_set;
		int max = 0;
		FD_ZERO(&read_set);
		FD_ZERO(&write_set);

		for (int ti = 0; ti < 2; ti++) if (port[ti].s) {
			// read more only if buffer was flushed
			std::lock_guard<std::mutex> lock(m_lock);
			if (port[ti].buffer_len == port[ti].buffer_done) {
				FD_SET(port[ti].s, &read_set);
				max = port[ti].s > max ? port[ti].s : max;
			}
			if (!uart_pty_fifo_isempty(&port[ti].in)) {
				FD_SET(port[ti].s, &write_set);
				max = port[ti].s > max ? port[ti].s : max;
			}
		}

		// short, but not too short interval
		struct timeval timo = { 0, 500 };
		int ret = select(max+1, &read_set, &write_set, NULL, &timo);

		if (ret < 0)
			break;

		for (int ti = 0; ti < 2; ti++) if (port[ti].s) {
			if (FD_ISSET(port[ti].s, &read_set)) {
				std::lock_guard<std::mutex> lock(m_lock);
				ssize_t r = read(port[ti].s, port[ti].buffer,
									sizeof(port[ti].buffer)-1);
				port[ti].buffer_len = r;
				port[ti].buffer_done = 0;
				TRACE(if (!port[ti].tap)
						hdump("pty recv", port[ti].buffer, r);)
			}
			if (port[ti].buffer_done < port[ti].buffer_len) {
				// write them in fifo
				std::lock_guard<std::mutex> lock(m_lock);
				while (port[ti].buffer_done < port[ti].buffer_len &&
						!uart_pty_fifo_isfull(&port[ti].out)) {
					int index = port[ti].buffer_done++;
					TRACE(int wi = port[ti].out.write;)
					uart_pty_fifo_write(&port[ti].out,
							port[ti].buffer[index]);
					TRACE(printf("w %3d:%02x (%d/%d) %s\n",
								wi, port[ti].buffer[index],
								port[ti].out.read,
								port[ti].out.write,
								m_bXOn ? "XON" : "XOFF");)
				}
			}
			if (FD_ISSET(port[ti].s, &write_set)) {
				std::lock_guard<std::mutex> lock(m_lock);
				uint8_t buffer[512];
				// write them in fifo
				uint8_t * dst = buffer;
				while (!uart_pty_fifo_isempty(&port[ti].in) &&
						(size_t)(dst - buffer) < sizeof(buffer)) {
					*dst = uart_pty_fifo_read(&port[ti].in);
					dst++;
				}
				size_t len = dst - buffer;
				size_t r = write(port[ti].s, buffer, len);
				if (r!=len)
					fprintf(stderr,"Failed to write to PTY\n");
				TRACE(if (!port[ti].tap) hdump("pty send", buffer, r);)
			}
		}
		/* DO NOT call this, this create a concurency issue with the
		 * FIFO that can't be solved cleanly with a memory barrier
			uart_pty_flush_incoming(p);
		  */
	}
	return NULL;
}

void uart_pty::Init(struct avr_t * avr)
{
	_Init(avr,this);

	RegisterNotify(BYTE_IN, MAKE_C_CALLBACK(uart_pty,OnByteIn), this);

	int hastap = (getenv("SIMAVR_UART_TAP") && atoi(getenv("SIMAVR_UART_TAP"))) ||
			(getenv("SIMAVR_UART_XTERM") && atoi(getenv("SIMAVR_UART_XTERM"))) ;

	for (int ti = 0; ti < 1 + hastap; ti++) {
		int m, s;

		if (openpty(&m, &s, port[ti].slavename, NULL, NULL) < 0) {
			fprintf(stderr, "%s: Can't create pty: %s", __FUNCTION__, strerror(errno));
			return ;
		}
		struct termios tio;
		tcgetattr(m, &tio);
		cfmakeraw(&tio);
		tcsetattr(m, TCSANOW, &tio);
		port[ti].s = m;
		port[ti].tap = ti != 0;
		port[ti].crlf = ti != 0;
		printf("uart_pty_init %s on port *** %s ***\n",
				ti == 0 ? "bridge" : "tap", port[ti].slavename);
	}

	auto fRunCB =[](void * param) { uart_pty* p = (uart_pty*)param; return p->Run();};
	pthread_create(&m_thread, NULL, fRunCB, this);

}

// Shuts down the thread on destruction.
uart_pty::~uart_pty()
{
	if (!m_thread)
		return;
	puts(__func__);
	m_bQuit = true; // Signal thread it's time to quit.
	for (int ti = 0; ti < 2; ti++)
		if (port[ti].s)
			close(port[ti].s);
	pthread_join(m_thread, NULL);
}

void uart_pty::Connect(char uart)
{
	// disable the stdio dump, as we are sending binary there
	uint32_t f = 0;
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_GET_FLAGS(uart), &f);
	f &= ~AVR_UART_FLAG_STDIO;
	avr_ioctl(m_pAVR, AVR_IOCTL_UART_SET_FLAGS(uart), &f);

	avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(uart), UART_IRQ_OUTPUT);
	avr_irq_t * dst = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(uart), UART_IRQ_INPUT);
	avr_irq_t * xon = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(uart), UART_IRQ_OUT_XON);
	avr_irq_t * xoff = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(uart), UART_IRQ_OUT_XOFF);
	if (src && dst) {
		ConnectFrom(src, BYTE_IN);
		ConnectTo(BYTE_OUT, dst);
	}
	if (xon)
		avr_irq_register_notify(xon, MAKE_C_CALLBACK(uart_pty,OnXOnIn), this);
	if (xoff)
		avr_irq_register_notify(xoff, MAKE_C_CALLBACK(uart_pty,OnXOffIn),this);

	for (int ti = 0; ti < 1; ti++)
		if (port[ti].s) {
			char link[128];
			sprintf(link, "/tmp/simavr-uart%s%c", ti == 1 ? "tap" : "", uart);
			unlink(link);
			if (symlink(port[ti].slavename, link) != 0) {
				fprintf(stderr, "WARN %s: Can't create %s: %s", __func__, link, strerror(errno));
			} else {
				printf("%s: %s now points to %s\n", __func__, link, port[ti].slavename);
			}
		}
	if (getenv("SIMAVR_UART_XTERM") && atoi(getenv("SIMAVR_UART_XTERM"))) {
		char cmd[256];
		sprintf(cmd, "xterm -e picocom -b 115200 %s >/dev/null 2>&1 &",
				tap.slavename);
		if (system(cmd)<0)
			printf("Could not launch xterm\n");
	} else
		printf("note: export SIMAVR_UART_XTERM=1 and install picocom to get a terminal\n");
}
