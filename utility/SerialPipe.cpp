/*
	SerialPipe.cpp - Connects two UART_PTYs together, aka null modem cable.

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

#include "SerialPipe.h"
#include "gsl-lite.hpp"
#include <cerrno>       // for EAGAIN, errno
#include <cstdio>
#include <fcntl.h>       // for open, O_NONBLOCK, O_RDWR
#include <iostream>       // for fprintf, printf, perror, NULL, stderr
#include <sys/select.h>  // for FD_ISSET, FD_SET, select, FD_ZERO, fd_set
#include <unistd.h>      // for read, write, close
#include <utility>

using std::cout;
using std::cerr;
using std::string;

SerialPipe::SerialPipe(string strUART0, string strUART1):m_strPty0(std::move(strUART0)),m_strPty1(std::move(strUART1))
{
	auto fcnThread = [](void *param){ auto *p = static_cast<SerialPipe*>(param); return p->Run(); };

	pthread_create(&m_thread, nullptr, fcnThread, this);
	m_bStarted = true;
}

SerialPipe::~SerialPipe()
{
	if (!m_bStarted)
	{
		return;
	}
	m_bQuit = true;
	pthread_cancel(m_thread);
	pthread_join(m_thread,nullptr);
	std::cout << "Serial pipe finished\n";
}

void* SerialPipe::Run()
{
	// Not much to see here, we just open the ports and shuttle characters back and forth across them.
	//printf("Starting serial transfer thread...\n");
	int _fdPort[2] ={0,0};
	gsl::span<int> fdPort(_fdPort);
	fd_set fdsIn {}, fdsErr {};
	unsigned char chrIn;
	int iLastFd = 0, iReadyRead, iChrRd;
	if ((fdPort[0]=open(m_strPty0.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC)) == -1) // NOLINT - no select alternative that uses iostream.
	{
		std::cerr << "Could not open "  << m_strPty0 << '\n';
		perror(m_strPty0.c_str());
		m_bQuit = true;
	}
	if ((fdPort[1]=open(m_strPty1.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC)) == -1) // NOLINT - no select alternative that uses iostream.
	{
		std::cerr << "Could not open "  << m_strPty1 << '\n';
		perror(m_strPty1.c_str());
		m_bQuit = true;
	}
	if (fdPort.at(0)>fdPort.at(1))
	{
		iLastFd = fdPort.at(0);
	}
	else
	{
		iLastFd = fdPort.at(1);
	}

	while (!m_bQuit)
	{
		FD_ZERO(&fdsIn); //NOLINT // complaints in system file.
		FD_ZERO(&fdsErr); //NOLINT
		FD_SET(fdPort.at(0), &fdsIn); //NOLINT
		FD_SET(fdPort.at(1), &fdsIn); //NOLINT
		FD_SET(fdPort.at(0), &fdsErr); //NOLINT
		FD_SET(fdPort.at(1), &fdsErr); //NOLINT
		if ((iReadyRead = select(iLastFd+1,&fdsIn, nullptr, &fdsErr,nullptr))<0)
		{
			std::cout << "Select ERR.\n";
			m_bQuit = true;
			break;
		}

		if (FD_ISSET(fdPort.at(0),&fdsIn)) //NOLINT
		{
			while ((iChrRd = read(fdPort.at(0), &chrIn,1))>0)
			{
				if(write(fdPort[1],&chrIn,1)!=1)
				{
					std::cerr << "Failed to write byte across serial pipe 0.\n";
				}

			}
			if (iChrRd == 0 || (iChrRd<0 && errno != EAGAIN))
			{
				m_bQuit = true;
				break;
			}
		}
		if (FD_ISSET(fdPort[1],&fdsIn)) //NOLINT
		{
			while ((iChrRd = read(fdPort[1], &chrIn,1))>0)
			{
				if(write(fdPort[0],&chrIn,1) !=1)
				{
					std::cerr << "Failed to write byte across serial pipe 0.\n";
				}
			}
			if (iChrRd == 0 || (iChrRd<0 && errno != EAGAIN))
			{
				m_bQuit = true;
				break;
			}
		}
		if (FD_ISSET(fdPort[0], &fdsErr) || FD_ISSET(fdPort[1], &fdsErr)) //NOLINT
		{
			std::cerr << "Exception reading PTY. Quit.\n";
			m_bQuit = true;
			break;
		}

	}

	// cleanup.
	for (auto &p: fdPort)
	{
		close(p);
	}
	return nullptr;
}
