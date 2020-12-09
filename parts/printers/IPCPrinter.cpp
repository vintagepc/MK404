/*
	Part_Test.cpp - Printer definition for the part test printer
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

#include "IPCPrinter.h"
#include "GLMotor.h"
#include <GL/glew.h> //NOLINT - must come first.
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


static constexpr char IPC_FILE[] = "MK404.IPC";

IPCPrinter::~IPCPrinter()
{
	unlink(IPC_FILE);
}

void IPCPrinter::SetupHardware()
{
	struct stat tmp;
	if (stat(IPC_FILE, &tmp)==0)
	{
		unlink (IPC_FILE);
	}
	if (mkfifo(IPC_FILE, 0666)<0)
	{
		std::cerr << "Error - could not create FIFO pipe. Aborting.\n";
		exit(2);
	}
	m_ifIn.open(IPC_FILE);
	// TODO... add motors.
}

void IPCPrinter::OnAVRCycle()
{

	uint8_t len = m_ifIn.get();
	m_len = m_ifIn.readsome(m_msg.data(),len);
	m_msg[len] = '\0';

	if (m_len<len) // client has gone away, message truncated.
	{
		if(m_ifIn.eof())
		{
			m_ifIn.close();
			m_ifIn.clear();
			m_vMotors.clear(); // clear objects.
			m_vInds.clear();
			m_ifIn.open(IPC_FILE);
		}
		std::cout << "Truncated message, ignoring.\n";
		return;
	}
	//std::cout << "msg len: " << std::to_string(len) << ':' << m_msg.data() << '\n';

	// std::cout << "Got line:'" <<m_msg << "'\n";

	switch (m_msg.at(0))
	{
		case 'A': // Add directive.
		{
			switch (m_msg.at(1))
			{
				case 'M':
					m_vMotors.push_back(std::unique_ptr<GLMotor>(new GLMotor()));
					break;
				case 'I':
					m_vInds.push_back(std::unique_ptr<IPCIndicator>(new IPCIndicator(m_msg.at(2))));
			}
			SetSizeChanged();
			// std::cout << "Total: " << std::to_m_msgg(m_vMotors.size()) <<  " motors\n";
		}
		break;
		case 'M': // Motor directive.
		{
			UpdateMotor();
		}
		break;
		case 'I':
			UpdateIndicator();
			break;
	}



}

void IPCPrinter::UpdateIndicator()
{
	uint8_t index = m_msg.at(1)-'0';
	if (index<0 || index>=m_vInds.size())
	{
		std::cerr << "Error: IPC invalid indicator index.\n";
		return;
	}
	switch (m_msg.at(2))
	{
		case 'V': // Value
		{
			m_vInds.at(index)->SetValue(m_msg.at(3));
		}
		break;
		case 'L': // Label
		{
			m_vInds.at(index)->SetLabel(m_msg.at(3));
		}
		break;
		case 'C':
		{
			uint32_t color = 0;
			for (int i=3; i<7; i++)
			{
				color <<=8;
				color |= static_cast<uint8_t>(m_msg.at(i));
			}
			m_vInds.at(index)->SetColor(color);
		}
		break;
	}
}

// Parse motor IPC calls in the form M[index][Cmd][Value].
// index is ascii value of the character relative to '0'.
// Cmd is one of the directive values to set
// Value depends on the option.
void IPCPrinter::UpdateMotor()
{
	uint8_t index = m_msg.at(1)-'0';
	if (index<0 || index>=m_vMotors.size())
	{
		std::cerr << "Error: IPC invalid motor index.\n";
		return;
	}
	switch (m_msg.at(2))
	{
		case 'E': // Enable - either 0 or 1, e.g. M1E0 or M1E1
		{
			m_vMotors.at(index)->SetEnable(m_msg.at(3)=='1');
			break;
		}
		case 'S': // Simple:
		{
			m_vMotors.at(index)->SetSimple(m_msg.at(3)=='1');
			break;
		}
		case 'L': // Label
		{
			m_vMotors.at(index)->SetAxis(m_msg.at(3));
			break;
		}
		case 'U': // Steps per mm. takes 4 bytes to make a uint32.
		{
			uint32_t steps = 0;
			for (int i=3; i<7; i++)
			{
				steps <<=8;
				steps |= static_cast<uint8_t>(m_msg.at(i));
			}
			m_vMotors.at(index)->SetStepsPerMM(steps);
			break;
		}
		case 'X': // maX - highest possible step count. takes 4 bytes to make an int32.
		{
			int32_t steps = 0;
			for (int i=3; i<7; i++)
			{
				steps <<=8;
				steps |= static_cast<uint8_t>(m_msg.at(i));
			}
		  	m_vMotors.at(index)->SetMaxPos(steps);
		 	break;
		}
		case 'P': // Position - takes 4 bytes to make a uint32.
		{
			if (m_msg.size()<7)
			{
				break;
			}
			int32_t steps = 0;
			for (int i=3; i<7; i++)
			{
				steps <<=8;
				steps |= static_cast<uint8_t>(m_msg.at(i));
			}
			// printf("Current step: %d\n",steps);
			m_vMotors.at(index)->SetCurrentPos(steps);
			break;
		}

	}
}

void IPCPrinter::Draw()
{
		glLoadIdentity();
		glScalef(500/350,4,1);
		for (auto &motor : m_vMotors)
		{
			motor->Draw();
			glTranslatef(0,10,0);
		}
		for (auto &ind : m_vInds)
		{
			ind->Draw();
			glTranslatef(30,0,0);
		}
		m_gl.OnDraw();
}
