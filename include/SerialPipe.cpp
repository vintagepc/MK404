/*
	SerialPipe.cpp - Connects two UART_PTYs together, aka null modem cable.

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

#include "SerialPipe.h"
#include <errno.h>       // for EAGAIN, errno
#include <fcntl.h>       // for open, O_NONBLOCK, O_RDWR
#include <stdio.h>       // for fprintf, printf, perror, NULL, stderr
#include <sys/select.h>  // for FD_ISSET, FD_SET, select, FD_ZERO, fd_set
#include <unistd.h>      // for read, write, close

SerialPipe::SerialPipe(std::string strUART0, std::string strUART1):m_strPty0(strUART0),m_strPty1(strUART1)
{
	auto fcnThread = [](void *param){ SerialPipe *p = (SerialPipe*)param; return p->Run(); };

	pthread_create(&m_thread, NULL, fcnThread, this);
	m_bStarted = true;
}

SerialPipe::~SerialPipe()
{
	if (!m_bStarted)
		return;
	m_bQuit = true;
	pthread_cancel(m_thread);
	pthread_join(m_thread,NULL);
	printf("Serial pipe finished\n");
}

void* SerialPipe::Run()
{
	// Not much to see here, we just open the ports and shuttle characters back and forth across them.
	printf("Starting serial transfer thread...\n");
	int fdPort[2];
	fd_set fdsIn, fdsErr;
	unsigned char chrIn;
	int iLastFd = 0, iReadyRead, iChrRd;
	if ((fdPort[0]=open(m_strPty0.c_str(), O_RDWR | O_NONBLOCK)) == -1)
	{
		fprintf(stderr, "Could not open %s.\n",m_strPty0.c_str());
		perror(m_strPty0.c_str());
		m_bQuit = true;
	}
	if ((fdPort[1]=open(m_strPty1.c_str(), O_RDWR | O_NONBLOCK)) == -1)
	{
		fprintf(stderr, "Could not open %s.\n",m_strPty1.c_str());
		perror(m_strPty1.c_str());
		m_bQuit = true;
	}
	if (fdPort[0]>fdPort[1])
		iLastFd = fdPort[0];
	else
		iLastFd = fdPort[1];

	while (!m_bQuit)
	{
		FD_ZERO(&fdsIn);
		FD_ZERO(&fdsErr);
		FD_SET(fdPort[0], &fdsIn);
		FD_SET(fdPort[1], &fdsIn);
		FD_SET(fdPort[0], &fdsErr);
		FD_SET(fdPort[1], &fdsErr);
		if ((iReadyRead = select(iLastFd+1,&fdsIn, NULL, &fdsErr,NULL))<0)
		{
			printf("Select ERR.\n");
			m_bQuit = true;
			break;
		}

		if (FD_ISSET(fdPort[0],&fdsIn))
		{
			while ((iChrRd = read(fdPort[0], &chrIn,1))>0)
			{
				if(write(fdPort[1],&chrIn,1)!=1)
					fprintf(stderr, "Failed to write byte across serial pipe 0.\n");

			}
			if (iChrRd == 0 || (iChrRd<0 && errno != EAGAIN))
			{
				m_bQuit = true;
				break;
			}
		}
		if (FD_ISSET(fdPort[1],&fdsIn))
		{
			while ((iChrRd = read(fdPort[1], &chrIn,1))>0)
			{
				if(write(fdPort[0],&chrIn,1) !=1)
					fprintf(stderr, "Failed to write byte across serial pipe 0.\n");
			}
			if (iChrRd == 0 || (iChrRd<0 && errno != EAGAIN))
			{
				m_bQuit = true;
				break;
			}
		}
		if (FD_ISSET(fdPort[0], &fdsErr) || FD_ISSET(fdPort[1], &fdsErr))
		{
			fprintf(stderr,"Exception reading PTY. Quit.\n");
			m_bQuit = true;
			break;
		}

	}

	// cleanup.
	for (int i=0; i<2; i++)
		close(fdPort[i]);
	return nullptr;
}
