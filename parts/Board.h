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
#include "Einsy_EEPROM.h"
#include <pthread.h>


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

			virtual ~Board(){};

			void CreateBoard();
			void StartAVR();
			void StopAVR();

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

			inline bool TryConnect(avr_irq_t *src, PinNames::Pin ePin)
			{
				if (m_wiring.IsPin(ePin))
				{
					avr_connect_irq(src, m_wiring.DIRQLU(m_pAVR,ePin));
					return true;
				}
				return false;
			}

		protected:
			// Define this method and use it to initialize/attach your hardware to the MCU.
			virtual void SetupHardware() = 0;

			// Overload this if you wish to have custom initialization code.
			// The Board class takes care of persisting flash and EEPROM for you.
			virtual void OnAVRInit(){};

			// Overload if you wish to run custom code on AVR shutdown
			virtual void OnAVRDeinit(){};

			// Called when the AVR is reset or powered up (i.e. MCUSR set)
			virtual void OnAVRReset(){};

			virtual void* RunAVR()
			{
				avr_regbit_t MCUSR = m_pAVR->reset_flags.porf;
				MCUSR.mask =0xFF;
				MCUSR.bit = 0;
				printf("Starting %s execution...\n", m_wiring.GetMCUName().c_str());
				int state = cpu_Running;
				while ((state != cpu_Done) && (state != cpu_Crashed) && !m_bQuit){
					int8_t uiMCUSR = avr_regbit_get(avr,MCUSR);
					if (uiMCUSR != m_uiLastMCUSR)
					{
						printf("MCUSR: %02x\n",m_uiLastMCUSR = uiMCUSR);
						if (uiMCUSR) // only run on change and not changed to 0
							OnAVRReset();
					}
					if (m_bReset)
					{
						m_bReset = 0;
						avr_reset(m_pAVR);
					}
					state = avr_run(m_pAVR);
				}
				avr_terminate(m_pAVR);
				printf("%s finished.\n",m_wiring.GetMCUName().c_str());
				return nullptr;
			};

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

			inline avr_irq_t* GetDIRQ(PinNames::Pin ePin)
			{
				if (m_wiring.IsPin(ePin))
					return m_wiring.DIRQLU(m_pAVR,ePin);
				return nullptr;
			}

			inline std::string GetStorageFileName(std::string strType)
			{
				std::string strFN = m_strBoard;
				strFN.append("_").append(m_wiring.GetMCUName()).append("_").append(strType).append(".bin");
				return strFN;
			}

			inline avr_irq_t* GetPWMIRQ(PinNames::Pin ePin)
			{
				if (m_wiring.IsPin(ePin))
					return m_wiring.DPWMLU(m_pAVR,ePin);
				return nullptr;
			}

			template<class HW, typename ...types>
			inline void AddHardware(HW &hw, types ... args)
			{
				hw.Init(m_pAVR, args... );
			}

			inline void SetBoardName(std::string strName){m_strBoard = strName;}

			struct avr_t* m_pAVR = nullptr;

		private:
			void CreateAVR();

			void _OnAVRInit();

			void _OnAVRDeinit();

			avr_flashaddr_t LoadFirmware(std::string strFW);

			int m_fdFlash = 0, m_fdEEPROM = 0;

			bool m_bQuit = false, m_bReset = false;
			pthread_t m_thread = 0;
			const Wiring &m_wiring;
			const std::string m_strFW, m_strBoot;
			std::string m_strBoard;
			const uint32_t m_uiFreq;

			uint8_t m_uiLastMCUSR = 0;

			// Loads an ELF or HEX file into the MCU. Returns boot PC

			Einsy_EEPROM m_EEPROM;
	};
};// Boards
#endif //__BOARD_H__
