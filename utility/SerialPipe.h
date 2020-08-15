/*
	SerialPipe.h - Connects two UART_PTYs together.

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

#pragma once

#include <pthread.h>  // for pthread_t
#include <string>     // for string
#include <atomic>

class SerialPipe
{
    public:
	// Constructs a new serial pipe betwen the named UARTs (typically /tmp/simavr-uart#)
	SerialPipe(std::string strUART0, std::string strUART1);

	// Destructor, shuts down the pipe thread.
	~SerialPipe();

    private:
		// Main thread function.
		void* Run();

		bool m_bStarted = false;
		std::atomic_bool m_bQuit = {false};
		pthread_t m_thread = 0;

		std::string m_strPty0, m_strPty1;

};
