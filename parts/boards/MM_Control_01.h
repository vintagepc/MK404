/*
	MM_Control_01.h - Board definition for the Prusa MMU2
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

#ifndef __BOARD_MM_CONTROL_01_H__
#define __BOARD_MM_CONTROL_01_H__

#include <Board.h>

#include "uart_pty.h"
#include "ADC_Buttons.h"
#include "HC595.h"
#include "LED.h"
#include "TMC2130.h"

#include <wiring/MM_Control_01.h>
namespace Boards
{
	class MM_Control_01: public Board
	{
		public:
			MM_Control_01(std::string strFW = "MM-control-01.hex", uint32_t uiFreq = 16000000)
				:Board(m_wiring,strFW,uiFreq){};

		protected:
			void SetupHardware() override;

		//	void CustomAVRInit() override;

		//	void CustomAVRDeinit() override;

			uart_pty m_UART;
			HC595 m_shift;
			TMC2130 m_Sel, m_Idl, m_Extr;
			LED m_lGreen[5], m_lRed[5], m_lFINDA;
			ADC_Buttons m_buttons;

		private:
			const Wirings::MM_Control_01 m_wiring;
	};
};
#endif