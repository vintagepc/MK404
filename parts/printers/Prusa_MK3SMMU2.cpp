/*
	Prusa_MK3SMMU2.cpp - Printer definition for the Prusa MK3S w/MMU2
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


#include "Prusa_MK3SMMU2.h"
#include "BasePeripheral.h"       // for MAKE_C_CALLBACK
#include "IRSensor.h"             // for IRSensor, IRSensor::IRState::IR_AUTO
#include "MK3SGL.h"               // for MK3SGL
#include "PinNames.h"             // for Pin::MMU_HWRESET
#include "SerialPipe.h"           // for SerialPipe
#include "printers/Prusa_MK3S.h"  // for Prusa_MK3S
#include "uart_pty.h"             // for uart_pty

Prusa_MK3SMMU2::~Prusa_MK3SMMU2()
{
	delete m_pipe;
}

void Prusa_MK3SMMU2::SetupHardware()
{
	Prusa_MK3S::SetupHardware();
	TryConnect(MMU_HWRESET,m_MMU,MMU2::RESET);
	IR.Set(IRSensor::IR_AUTO);
	avr_irq_register_notify(m_MMU.GetIRQ(MMU2::FEED_DISTANCE), MAKE_C_CALLBACK(Prusa_MK3SMMU2,OnMMUFeed),this);

	// Note we can't directly connect the MMU or you'll get serial flow issues/lost bytes.
	// The serial_pipe thread lets us reuse the UART_PTY code and its internal xon/xoff/buffers
	// rather than having to roll our own internal FIFO. As an added bonus you can tap the ports for debugging.
	m_pipe = new SerialPipe(UART2.GetSlaveName(), m_MMU.GetSerialPort());
}

void Prusa_MK3SMMU2::OnVisualTypeSet(VisualType type)
{
	if (type==VisualType::MINIMAL)
		return;
	Prusa_MK3S::OnVisualTypeSet(type);
	// Wire up the additional MMU stuff.

	AddHardware(m_sniffer,'2');
	m_pVis->ConnectFrom(m_sniffer.GetIRQ(GCodeSniffer::CODEVAL_OUT),MK3SGL::TOOL_IN);
	m_pVis->ConnectFrom(m_MMU.GetIRQ(MMU2::SELECTOR_OUT), MK3SGL::SEL_IN);
	m_pVis->ConnectFrom(m_MMU.GetIRQ(MMU2::IDLER_OUT), MK3SGL::IDL_IN);
	m_pVis->ConnectFrom(m_MMU.GetIRQ(MMU2::LEDS_OUT),MK3SGL::MMU_LEDS_IN);
	m_pVis->ConnectFrom(m_MMU.GetIRQ(MMU2::FINDA_OUT),MK3SGL::FINDA_IN);
	m_pVis->ConnectFrom(m_MMU.GetIRQ(MMU2::FEED_DISTANCE), MK3SGL::FEED_IN);
}

std::pair<int,int> Prusa_MK3SMMU2::GetWindowSize()
{
	auto prSize = Prusa_MK3S::GetWindowSize();
	prSize.second +=50;
	return prSize;
}

void Prusa_MK3SMMU2::Draw()
{
	m_MMU.Draw();
	Prusa_MK3S::Draw();
}

// Helper for MMU IR sensor triggering.
void Prusa_MK3SMMU2::OnMMUFeed(struct avr_irq_t * irq, uint32_t value)
{
	float *fVal = (float*)&value;
	IR.Auto_Input(fVal[0]>400); // Trigger IR if MMU P pos > 400mm
}


void Prusa_MK3SMMU2::OnKeyPress(unsigned char key, int x, int y)
{
	switch (key) {
		case 'F':
		{
			printf("FINDA toggled (in manual control)\n");
			m_MMU.SetFINDAAuto(false);
			m_MMU.ToggleFINDA();
		}
		break;
		case 'a':
		{
			printf("FINDA in Auto control\n");
			m_MMU.SetFINDAAuto(true);
			IR.Set(IRSensor::IR_AUTO); // Also restore IR auto handling.
			break;
		}
		case '3':
		case '4':
		case '5':
			m_MMU.PushButton(key - '2'); // button numbers are 1/2/3
			break;
		default:
			Prusa_MK3S::OnKeyPress(key,x,y);
	}
}
