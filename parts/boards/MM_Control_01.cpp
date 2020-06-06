/*
	MM_Control_01.cpp - Board definition for the Prusa MMU2
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

#include <boards/MM_Control_01.h>

namespace Boards
{
	
	void MM_Control_01::SetupHardware()
	{
		DisableInterruptLevelPoll(5);

		AddSerialPty(m_UART,'1');

		m_shift.Init(m_pAVR);
		TryConnect(SHIFT_LATCH,m_shift,HC595::IN_LATCH);
		TryConnect(SHIFT_DATA,m_shift,HC595::IN_DATA);
		TryConnect(SHIFT_CLOCK,m_shift,HC595::IN_CLOCK);

		TMC2130::TMC2130_cfg_t cfg;
		cfg.uiStepsPerMM = 25;
		cfg.fStartPos = 0;
		cfg.cAxis = 'P';
		cfg.bHasNoEndStops = true;
		
		m_Extr.SetConfig(cfg);
		m_Extr.Init(m_pAVR);
		TryConnect(P_TMC2130_CS, m_Extr, TMC2130::SPI_CSEL);
		TryConnect(P_STEP_PIN, m_Extr, TMC2130::STEP_IN);
		TryConnect(m_Extr,TMC2130::DIAG_OUT,P_TMC2130_DIAG);
		SetPin(P_TMC2130_DIAG,0);
		m_Extr.ConnectFrom(m_shift.GetIRQ(HC595::BIT0),	TMC2130::DIR_IN);
		m_Extr.ConnectFrom(m_shift.GetIRQ(HC595::BIT1),	TMC2130::ENABLE_IN);

		cfg.uiStepsPerMM = 50;
		cfg.cAxis = 'S';
		cfg.iMaxMM = 70;
		cfg.bInverted = true;
		cfg.bHasNoEndStops = false;
		m_Sel.SetConfig(cfg);
		m_Sel.Init(m_pAVR);
		TryConnect(S_TMC2130_CS,m_Sel,TMC2130::SPI_CSEL);
		TryConnect(S_STEP_PIN,m_Sel,TMC2130::STEP_IN);
		TryConnect(m_Sel,TMC2130::DIAG_OUT,S_TMC2130_DIAG);
		SetPin(S_TMC2130_DIAG,0);
		m_Sel.ConnectFrom(m_shift.GetIRQ(HC595::BIT2), TMC2130::DIR_IN);
		m_Sel.ConnectFrom(m_shift.GetIRQ(HC595::BIT3), TMC2130::ENABLE_IN);

		cfg.uiStepsPerMM = 8;
		cfg.iMaxMM = 200;
		cfg.cAxis = 'I';
		m_Idl.SetConfig(cfg);
		m_Idl.Init(m_pAVR);
		TryConnect(I_TMC2130_CS,m_Idl,TMC2130::SPI_CSEL);
		TryConnect(I_STEP_PIN,m_Idl,TMC2130::STEP_IN);
		TryConnect(m_Idl,TMC2130::DIAG_OUT,I_TMC2130_DIAG);
		SetPin(I_TMC2130_DIAG,0);
		m_Idl.ConnectFrom(m_shift.GetIRQ(HC595::BIT5), TMC2130::ENABLE_IN);
		m_Idl.ConnectFrom(m_shift.GetIRQ(HC595::BIT4), TMC2130::DIR_IN);
		

		for (int i=0; i<5; i++)
		{
			m_lGreen[i] = LED(0x00FF00FF);
			m_lGreen[i].Init(m_pAVR);
			m_lRed[i] = LED(0xFF0000FF);
			m_lRed[i].Init(m_pAVR);
		}
		m_lFINDA = LED(0xFFCC00FF,'F');
		m_lFINDA.Init(m_pAVR);
		TryConnect(FINDA_PIN,m_lFINDA,LED::LED_IN);
		SetPin(FINDA_PIN,0);

		m_lGreen[0].ConnectFrom(m_shift.GetIRQ(HC595::BIT6), LED::LED_IN);
		m_lRed[0].ConnectFrom(	m_shift.GetIRQ(HC595::BIT7), LED::LED_IN);
		m_lGreen[4].ConnectFrom(m_shift.GetIRQ(HC595::BIT8), LED::LED_IN);
		m_lRed[4].ConnectFrom(	m_shift.GetIRQ(HC595::BIT9), LED::LED_IN);
		m_lGreen[3].ConnectFrom(m_shift.GetIRQ(HC595::BIT10), LED::LED_IN);
		m_lRed[3].ConnectFrom(	m_shift.GetIRQ(HC595::BIT11), LED::LED_IN);
		m_lGreen[2].ConnectFrom(m_shift.GetIRQ(HC595::BIT12), LED::LED_IN);
		m_lRed[2].ConnectFrom(	m_shift.GetIRQ(HC595::BIT13), LED::LED_IN);
		m_lGreen[1].ConnectFrom(m_shift.GetIRQ(HC595::BIT14), LED::LED_IN);
		m_lRed[1].ConnectFrom(	m_shift.GetIRQ(HC595::BIT15), LED::LED_IN);

		m_buttons.Init(m_pAVR,5);

	}



};