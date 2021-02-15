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

#define PIPE 0
#define MQ 0

#define SHM 1

static constexpr char IPC_FILE[] = "/MK404IPC";

#if MQ
static constexpr mqd_t MQ_ERR = -1;
#endif

#if SHM

#endif

IPCPrinter::~IPCPrinter()
{
#if PIPE
	unlink(IPC_FILE);
#endif
#if MQ
	if (mq_close(m_queue)!=MQ_ERR)
	{
		mq_unlink(IPC_FILE);
	}
#endif
	if (m_queue)
	{
		shmemq_destroy(m_queue,1);
	}
}

void IPCPrinter::SetupHardware()
{
#if MQ
	m_qAttr.mq_msgsize =7;
	m_qAttr.mq_maxmsg = 50;
	m_qAttr.mq_curmsgs = 0;
	m_qAttr.mq_flags = 0;
	m_queue = mq_open(IPC_FILE, O_CREAT | O_RDONLY, 0644, &m_qAttr);
	if (m_queue == MQ_ERR)
	{
		std::cerr << " ## Failed to open the message queue! ## \n";
		perror(IPC_FILE);
		exit(1);
	}
#elif PIPE
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
#elif SHM
	m_queue = shmemq_create(IPC_FILE);
#endif
	_Init(Board::m_pAVR,this);
}

void IPCPrinter::OnAVRCycle()
{


#if MQ
	ssize_t len = mq_receive(m_queue, m_msg.data(), m_msg.size_bytes()-1, nullptr);
	if (len == 0 || m_msg.at(0)=='C')
	{
			m_vMotors.clear(); // clear objects.
			m_vInds.clear();
			m_vStepIRQs.clear();
			m_pVis->ClearPrint();
			return;
	}
#elif PIPE
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
			m_vStepIRQs.clear();
			m_ifIn.open(IPC_FILE);
		}
		std::cout << "Truncated message, ignoring.\n";
		return;
	}
	//std::cout << "msg len: " << std::to_string(len) << ':' << m_msg.data() << '\n';

	// std::cout << "Got line:'" <<m_msg << "'\n";
#elif SHM
	if (!shmemq_try_dequeue(m_queue, &_m_msg))
	{
		usleep(100);
		return;
	}
#endif
	switch (m_msg.at(0))
	{
		case 'C':
		{
			m_vMotors.clear(); // clear objects.
			m_vInds.clear();
			m_vStepIRQs.clear();
			if (m_pVis) {
				m_pVis->ClearPrint();
			}
		}
		case 'M': // Motor directive.
		{
			UpdateMotor();
		}
		break;
		case 'I':
			UpdateIndicator();
			break;
		case 'A': // Add directive.
		{
			switch (m_msg.at(1))
			{
				case 'M':
					m_vMotors.push_back(std::unique_ptr<GLMotor>(new GLMotor()));
					m_vStepIRQs.push_back(COUNT);
					break;
				case 'I':
					m_vInds.push_back(std::unique_ptr<IPCIndicator>(new IPCIndicator(m_msg.at(2))));
					switch (m_msg.at(2))
					{
						case 'M': // MINDA:
							m_vIndIRQs.push_back(PINDA_OUT);
							break;
						case 'B':
							m_vIndIRQs.push_back(BED_OUT);
							break;
						default:
							m_vIndIRQs.push_back(COUNT);
							break;
					}
					break;
			}
			SetSizeChanged();
			// std::cout << "Total: " << std::to_m_msgg(m_vMotors.size()) <<  " motors\n";
		}
		break;
	}

}

void IPCPrinter::OnVisualTypeSet(const std::string &type)
{
	if (type!="lite")
	{
		return;
	}

	m_pVis.reset(new MK3SGL(type,false,this)); //NOLINT - suggestion is c++14.

	AddHardware(*m_pVis);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::X_POSITION_OUT),MK3SGL::X_IN);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::Y_POSITION_OUT),MK3SGL::Y_IN);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::Z_POSITION_OUT),MK3SGL::Z_IN);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::E_POSITION_OUT),MK3SGL::E_IN);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::X_STEP_OUT),MK3SGL::X_STEP_IN);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::Y_STEP_OUT),MK3SGL::Y_STEP_IN);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::Z_STEP_OUT),MK3SGL::Z_STEP_IN);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::E_STEP_OUT),MK3SGL::E_STEP_IN);
	// m_pVis->ConnectFrom(pinda.GetIRQ(PINDA::SHEET_OUT), MK3SGL::SHEET_IN);
	// m_pVis->ConnectFrom(fExtruder.GetIRQ(Fan::ROTATION_OUT), MK3SGL::EFAN_IN);
	// m_pVis->ConnectFrom(fPrint.GetIRQ(Fan::ROTATION_OUT), MK3SGL::PFAN_IN);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::BED_OUT), MK3SGL::BED_IN);
	// m_pVis->ConnectFrom(sd_card.GetIRQ(SDCard::CARD_PRESENT), MK3SGL::SD_IN);
	m_pVis->ConnectFrom(GetIRQ(IPCPrinter::PINDA_OUT), MK3SGL::PINDA_IN);
	// m_pVis->SetLCD(&lcd);
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
			if (m_vIndIRQs.at(index)!=COUNT)
			{
				// printf("IND %c set to %d\n",m_msg.at(1), m_msg.at(3));
				RaiseIRQ(m_vIndIRQs.at(index),m_msg.at(3)>0);
			}
		}
		break;
		case 'L': // Label
		{
			m_vInds.at(index)->SetLabel(m_msg.at(3));
			switch (m_msg.at(3))
			{
				case 'M': // MINDA:
					m_vIndIRQs[index] =PINDA_OUT;
					break;
				case 'B':
					m_vIndIRQs[index] = BED_OUT;
					break;
				default:
					m_vIndIRQs[index] = COUNT;
					break;
			}
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
		case 'P': // Position - takes 4 bytes to make a uint32.
		{
			if (m_msg.size()<7)
			{
				break;
			}
			int32_t steps = 0;
			// for (int i=3; i<7; i++)
			// {
			// 	steps <<=8;
			// 	steps |= static_cast<uint8_t>(m_msg.at(i));
			// }
			std::memcpy(&steps, m_msg.begin()+3, sizeof(steps)); // both 32 bits, just mangle it for sending over the wire.
			// if (index==3) printf("Current step: %d\n",steps);
			m_vMotors.at(index)->SetCurrentPos(steps);
			if (m_vStepIRQs.at(index)!=COUNT)
			{
				RaiseIRQ(m_vStepIRQs.at(index),steps);
				float fPos = m_vMotors.at(index)->GetCurrentPos();
				uint32_t posOut = 0;
				std::memcpy(&posOut, &fPos, sizeof(posOut)); // both 32 bits, just mangle it for sending over the wire.
				RaiseIRQ(m_vStepIRQs.at(index)+1, posOut);
			}
			break;
		}
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
			switch (m_msg.at(3))
			{
				case 'X':
					m_vStepIRQs[index] = X_STEP_OUT;
					break;
				case 'Y':
					m_vStepIRQs[index] = Y_STEP_OUT;
					break;
				case 'Z':
					m_vStepIRQs[index] = Z_STEP_OUT;
					break;
				case 'E':
					m_vStepIRQs[index] = E_STEP_OUT;
					break;
				default:
					m_vStepIRQs[index] = COUNT;
					break;
			}
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
			bool bUpdate = true;
			switch (m_vMotors.at(index)->m_cAxis.load())
			{
				case 'X':
					m_vStepsPerMM[0] = steps;
					break;
				case 'Y':
					m_vStepsPerMM[1] = steps;
					break;
				case 'Z':
					m_vStepsPerMM[2] = steps;
					break;
				case 'E':
					m_vStepsPerMM[3] = steps;
					break;
				default:
					bUpdate = false;
					break;
			}
			if (bUpdate && m_pVis)
			{
				m_pVis->SetStepsPerMM(m_vStepsPerMM.at(0),m_vStepsPerMM.at(1),m_vStepsPerMM.at(2),m_vStepsPerMM.at(3));
			}
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
	if ((GetVisualType()!="none") && m_pVis)
	{
		m_pVis->FlagForRedraw();
	}
}
