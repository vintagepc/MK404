/*
	Board.h - Base class for a "board"
	Derive from this and set the values accordingly for your model, -1 is undefined/unused.

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

#ifndef __BOARD_H__
#define __BOARD_H__

#include "Wiring.h"
#include <BasePeripheral.h>
#include <avr_extint.h>
#include <uart_pty.h>

using namespace PinNames;
using namespace Wirings;
using namespace std;
namespace Boards 
{
	class Board
	{
		public:
			// Making a type so that this is easy to update 
			// down the road if we need more values than this can provide.
			typedef signed char MCUPin;

			// Creates a new board with the given pinspec, firmware file, frequency, and (optional) bootloader hex
				Board(const Wiring &wiring, string strFW, uint32_t uiFreqHz, string strBoot = ""):
					m_wiring(wiring),m_strFW(strFW),m_uiFreq(uiFreqHz),m_strBoot(strBoot){};

			void CreateBoard()
			{
				CreateAVR();
				m_pAVR->pc = LoadFirmware(m_strFW);
				if (!m_strBoot.empty())
				{
					m_pAVR->reset_pc = LoadFirmware(m_strBoot);
				}
				m_pAVR->frequency = m_uiFreq;
				m_pAVR->vcc = 5000;
				m_pAVR->aref = 0;
				m_pAVR->avcc = 5000;
				//m_pAVR->log = 1 + verbose;

				// even if not setup at startup, activate gdb if crashing
				m_pAVR->gdb_port = 1234;
				SetupHardware();
			};

			virtual ~Board(){};

			// Returns the AVR core. 
			inline avr_t * GetAVR(){return m_pAVR;}

			template <class HW>
			inline bool TryConnect(PinNames::Pin ePin, HW &hw, unsigned int eDest)
			{ 
				if (m_wiring.IsPin(ePin))
				{
					hw.ConnectFrom(m_wiring.DIRQLU(m_pAVR,ePin),eDest);
					return true;
				}
				else
					return false;
			};

			template <class HW>
			inline bool TryConnect(HW &hw, unsigned int eDest,PinNames::Pin ePin)
			{ 
				if (m_wiring.IsPin(ePin))
				{
					hw.ConnectTo(eDest,m_wiring.DIRQLU(m_pAVR,ePin));
					return true;
				}
				else
					return false;
			};

		protected:
			// Define this method and use it to initialize/attach your hardware to the MCU.
			virtual void SetupHardware() = 0;

			// Overload this if you wish to have custom initialization code. 
			// The Board class takes care of persisting flash and EEPROM for you.
			virtual void CustomAVRInit(){};

			// Overload if you wish to run custom code on AVR shutdown
			virtual void CustomAVRDeinit(){};

			

			// suppress continuous polling for low INT lines... major performance drain.
			void DisableInterruptLevelPoll(uint8_t uiNumIntLins)
			{
				for (int i=0; i<uiNumIntLins; i++)
					avr_extint_set_strict_lvl_trig(m_pAVR,i,false);

			}

			inline void AddSerialPty(uart_pty &UART, const char chrNum)
			{
				UART.Init(m_pAVR);
				UART.Connect(chrNum);
			}

			inline void SetPin(PinNames::Pin ePin, uint32_t value)
			{
				avr_raise_irq(m_wiring.DIRQLU(m_pAVR,ePin),value);
			}

			struct avr_t* m_pAVR = nullptr;

		private:
			const Wiring &m_wiring;
			const std::string m_strFW, m_strBoot;
			const uint32_t m_uiFreq;
			// Loads an ELF or HEX file into the MCU. Returns boot PC
			avr_flashaddr_t LoadFirmware(std::string strFW);
			void CreateAVR();
	};
};// Boards
#endif //__BOARD_H__