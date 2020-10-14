/*
	Board.h - Base class for a "board"
	Derive from this and set the values accordingly for your model, -1 is undefined/unused.

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

#pragma once

#include "EEPROM.h"         // for EEPROM
#include "IKeyClient.h"
#include "IScriptable.h"    // for ArgType, IScriptable::LineStatus, IScript...
#include "PinNames.h"       // for Pin
#include "Scriptable.h"     // for Scriptable
#include "Util.h"
#include "Wiring.h"         // for Wiring
#include "sim_avr.h"        // for avr_t, avr_flashaddr_t, avr_reset, avr_run
#include "sim_irq.h"        // for avr_connect_irq, avr_irq_t, avr_raise_irq
#include <atomic>
#include <cstdint>         // for uint32_t, uint8_t, int8_t
#include <pthread.h>        // for pthread_join, pthread_t
#include <string>           // for string, basic_string, stoi
#include <utility>
#include <vector>           // for vector

using namespace PinNames; //NOLINT - because proper using declarations don't support enums.
class uart_pty;
class BasePeripheral;

namespace Boards
{
	class Board : public Scriptable, virtual private IKeyClient
	{
		public:
			// Making a type so that this is easy to update
			// down the road if we need more values than this can provide.
			using MCUPin = signed char;

			// Creates a new board with the given pinspec, firmware file, frequency, and (optional) bootloader hex
			Board(const Wirings::Wiring &wiring,uint32_t uiFreqHz);

			~Board() override;

			void CreateBoard(const std::string &strFW, uint8_t uiVerbose, bool bGDB, uint32_t uiVCDRate, const std::string &strBoot = "stk500boot_v2_mega2560.hex");
			void StartAVR();
			void StopAVR();

			// Returns the AVR core.
			inline avr_t * GetAVR(){return m_pAVR;}

			bool TryConnect(PinNames::Pin ePin, BasePeripheral* hw, unsigned int eDest);

			bool TryConnect(BasePeripheral* hw, unsigned int eDest,PinNames::Pin ePin);

			bool TryConnect(avr_irq_t *src, PinNames::Pin ePin);

			// Start the bootloader first boot instead of jumping right into the main FW.
			inline void SetStartBootloader() {m_pAVR->pc = m_pAVR->reset_pc;}

			inline void WaitForFinish() {pthread_join(m_thread,nullptr);}

			inline void SetDisableWorkarounds(bool bVal){m_bNoHacks =bVal;}
			inline bool GetDisableWorkarounds(){return m_bNoHacks;}

			inline void SetSDCardFile(std::string strFile){m_strSDFile = std::move(strFile);}
			inline std::string GetSDCardFile(){return m_strSDFile.empty()?GetStorageFileName("SDcard"):m_strSDFile;}

			inline void SetResetFlag(){m_bReset = true;}
			inline void SetQuitFlag(){m_bQuit = true; m_bPaused = false;}
			inline bool GetQuitFlag(){return m_bQuit;}

			inline bool IsStopped(){ return m_pAVR->state == cpu_Stopped;}
			inline bool IsPaused(){ return m_bPaused;}

			// Is this the primary controlling board?
			inline void SetPrimary(bool bVal) { m_bIsPrimary = bVal;}

			// Should the board attempt to correct clock skew for fast simulation?
			inline void SetAdjustSkew(bool bVal) { m_bCorrectSkew = bVal;}

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

			void OnKeyPress(const Key& key) override;

			LineStatus ProcessAction(unsigned int ID, const std::vector<std::string> &vArgs) override;

			virtual void* RunAVR();

			// suppress continuous polling for low INT lines... major performance drain.
			void DisableInterruptLevelPoll(uint8_t uiNumIntLins);

			void AddSerialPty(uart_pty *UART, const char chrNum);

			void AddUARTTrace(const char chrUART);

			inline void SetPin(PinNames::Pin ePin, uint32_t value)
			{
				avr_raise_irq(m_wiring.DIRQLU(m_pAVR,ePin),value);
			}

			avr_irq_t* GetDIRQ(PinNames::Pin ePin);

			std::string GetStorageFileName(const std::string &strType);

			MCUPin GetPinNumber(PinNames::Pin ePin);

			avr_irq_t* GetPWMIRQ(PinNames::Pin ePin);

			template<class HW, typename ...types>
			inline void AddHardware(HW &hw, types ... args)
			{
				hw.Init(m_pAVR, args... );
			}

			inline void SetBoardName(std::string strName){m_strBoard = std::move(strName);}

			struct avr_t* m_pAVR = nullptr;

			std::atomic_uint8_t m_bPaused = {false};

			// Whether any vendor-specific hackery should be disabled.
			bool m_bNoHacks = false;

		private:
			void CreateAVR();

			void _OnAVRInit();

			void _OnAVRDeinit();

			bool _PinNotConnectedMsg(PinNames::Pin ePin);

			avr_flashaddr_t LoadFirmware(const std::string &strFW);

			std::atomic_bool m_bQuit = {false}, m_bReset = {false};
			bool m_bIsPrimary = false, m_bCorrectSkew = false;

			pthread_t m_thread = 0;
			const Wirings::Wiring &m_wiring;
			std::string m_strBoard = "";

			std::string m_strSDFile = "";
			const uint32_t m_uiFreq;

			uint8_t m_uiLastMCUSR = 0;

			unsigned int m_uiWtCycleCount = 0;

			avr_flashaddr_t m_bootBase{0}, m_FWBase{0};

			// Loads an ELF or HEX file into the MCU. Returns boot PC
			enum ScriptAction
			{
				Quit,
				Reset,
				Wait,
				Pause,
				Unpause
			};

			EEPROM m_EEPROM;
	};
}; // namespace Boards
