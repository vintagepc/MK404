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

#pragma once

#include <ScriptHost.h>     // for ScriptHost
#include <Scriptable.h>     // for Scriptable
#include <avr_extint.h>     // for avr_extint_set_strict_lvl_trig
#include <pthread.h>        // for pthread_join, pthread_t
#include <stdint.h>         // for uint32_t, uint8_t, int8_t
#include <stdio.h>          // for printf, fprintf, NULL, stderr
#include <uart_pty.h>       // for uart_pty
#include <unistd.h>         // for usleep
#include <string>           // for string, basic_string, stoi
#include <vector>           // for vector
#include "EEPROM.h"         // for EEPROM
#include "IScriptable.h"    // for ArgType, IScriptable::LineStatus, IScript...
#include "PinNames.h"       // for Pin
#include "Wiring.h"         // for Wiring
#include "sim_avr.h"        // for avr_t, avr_flashaddr_t, avr_reset, avr_run
#include "sim_avr_types.h"  // for avr_regbit_t
#include "sim_irq.h"        // for avr_connect_irq, avr_irq_t, avr_raise_irq
#include "sim_regbit.h"     // for avr_regbit_get, avr_regbit_set

using namespace PinNames;
using namespace Wirings;
using namespace std;
namespace Boards
{
	class Board : public Scriptable
	{
		public:
			// Making a type so that this is easy to update
			// down the road if we need more values than this can provide.
			typedef signed char MCUPin;

			// Creates a new board with the given pinspec, firmware file, frequency, and (optional) bootloader hex
			Board(const Wiring &wiring,uint32_t uiFreqHz):Scriptable("Board"),m_wiring(wiring),m_uiFreq(uiFreqHz)
			{
				RegisterAction("Quit", "Sends the quit signal to the AVR",ScriptAction::Quit);
				RegisterAction("Reset","Resets the board by resetting the AVR.", ScriptAction::Reset);
				RegisterAction("WaitMs","Waits the specified number of milliseconds (in AVR-clock time)", ScriptAction::Wait,{ArgType::Int});
			};

			virtual ~Board(){ if (m_thread) fprintf(stderr, "PROGRAMMING ERROR: %s THREAD NOT STOPPED BEFORE DESTRUCTION.\n",m_strBoard.c_str());};

			void CreateBoard(string strFW, uint8_t uiVerbose, bool bGDB, uint32_t uiVCDRate, string strBoot = "stk500boot_v2_mega2560.hex");
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
					return _PinNotConnectedMsg(ePin);
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
					return _PinNotConnectedMsg(ePin);
			};

			inline bool TryConnect(avr_irq_t *src, PinNames::Pin ePin)
			{
				if (m_wiring.IsPin(ePin))
				{
					avr_connect_irq(src, m_wiring.DIRQLU(m_pAVR,ePin));
					return true;
				}
				return _PinNotConnectedMsg(ePin);
			}

			// Start the bootloader first boot instead of jumping right into the main FW.
			inline void SetStartBootloader() {m_pAVR->pc = m_pAVR->reset_pc;}

			inline void WaitForFinish() {pthread_join(m_thread,NULL);}

			inline void SetDisableWorkarounds(bool bVal){m_bNoHacks =bVal;}
			inline bool GetDisableWorkarounds(){return m_bNoHacks;}

			inline void SetSDCardFile(string strFile){m_strSDFile = strFile;}
			inline string GetSDCardFile(){return m_strSDFile.empty()?GetStorageFileName("SDcard"):m_strSDFile;}

			inline void SetResetFlag(){m_bReset = true;}
			inline void SetQuitFlag(){m_bQuit = true;}

			inline bool IsStopped(){ return m_pAVR->state == cpu_Stopped;}
			inline bool IsPaused(){ return m_bPaused;}

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

			// Helper called every cycle - use it to process keys, mouse, etc.
			// within the context of the AVR run thread.
			virtual void OnAVRCycle(){};

			virtual LineStatus ProcessAction(unsigned int ID, const vector<string> &vArgs) override
			{
				switch (ID)
				{
					case Quit:
						SetQuitFlag();
						return LineStatus::Finished;
					case Reset:
						SetResetFlag();
						return LineStatus::Finished;
					case Wait:
						if (m_uiWtCycleCount >0)
							if (--m_uiWtCycleCount==0)
								return LineStatus::Finished;
							else
								return LineStatus::Waiting;
						else
						{
							m_uiWtCycleCount = (m_uiFreq/1000)*stoi(vArgs.at(0));
							return LineStatus::Waiting;
						}
				}
				return LineStatus::Unhandled;
			}

			virtual void* RunAVR()
			{
				avr_regbit_t MCUSR = m_pAVR->reset_flags.porf;
				MCUSR.mask =0xFF;
				MCUSR.bit = 0;
				printf("Starting %s execution...\n", m_wiring.GetMCUName().c_str());
				int state = cpu_Running;
				while ((state != cpu_Done) && (state != cpu_Crashed) && !m_bQuit){
							// Re init the special workarounds we need after a reset.
					if (m_bPaused)
					{
						usleep(100000);
						continue;
					}
					int8_t uiMCUSR = avr_regbit_get(m_pAVR,MCUSR);
					if (uiMCUSR != m_uiLastMCUSR)
					{
						printf("MCUSR: %02x\n",m_uiLastMCUSR = uiMCUSR);
						if (uiMCUSR) // only run on change and not changed to 0
							OnAVRReset();
					}
					OnAVRCycle();
					if (ScriptHost::IsInitialized())
						ScriptHost::OnAVRCycle();
					if (m_bReset)
					{
						m_bReset = 0;
						avr_reset(m_pAVR);
						avr_regbit_set(m_pAVR, m_pAVR->reset_flags.extrf);
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

			void AddUARTTrace(const char chrUART);

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

			inline MCUPin GetPinNumber(PinNames::Pin ePin)
			{
				if (m_wiring.IsPin(ePin))
					return m_wiring.GetPin(ePin);
				return -1;
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

			atomic_uint8_t m_bPaused = {false};

		private:
			void CreateAVR();

			void _OnAVRInit();

			void _OnAVRDeinit();

			inline bool _PinNotConnectedMsg(Pin ePin)
			{
				printf("Requested connection w/ Digital pin %d on %s, but it is not defined!\n",ePin,m_strBoard.c_str());
				return false;
			}

			avr_flashaddr_t LoadFirmware(std::string strFW);

			int m_fdFlash = 0;

			atomic_bool m_bQuit = {false}, m_bReset = {false};
			bool m_bNoHacks = false;
			pthread_t m_thread = 0;
			const Wiring &m_wiring;
			//std::string m_strFW, m_strBoot;
			std::string m_strBoard;

			std::string m_strSDFile = "";
			const uint32_t m_uiFreq;

			uint8_t m_uiLastMCUSR = 0;

			unsigned int m_uiWtCycleCount = 0;

			avr_flashaddr_t m_bootBase, m_FWBase;

			// Loads an ELF or HEX file into the MCU. Returns boot PC
			enum ScriptAction
			{
				Quit,
				Reset,
				Wait
			};


			EEPROM m_EEPROM;
	};
};// Boards
