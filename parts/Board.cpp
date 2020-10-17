/*
	Board.cpp - Base class for a "board"
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

#include "Board.h"
#include "BasePeripheral.h"  // for BasePeripheral
#include "KeyController.h"  // for KeyController
#include "ScriptHost.h"     // for ScriptHost
#include "TelemetryHost.h"
#include "Util.h"           // for CXXDemangle
#include "avr_extint.h"     // for avr_extint_set_strict_lvl_trig
#include "avr_uart.h"
#include "gsl-lite.hpp"
#include "sim_avr_types.h"  // for avr_regbit_t
#include "sim_elf.h"  // for avr_load_firmware, elf_firmware_t, elf_read_fir...
#include "sim_gdb.h"  // for avr_gdb_init
#include "sim_hex.h"  // for read_ihex_file
#include "sim_io.h"         // for avr_io_getirq
#include "sim_regbit.h"     // for avr_regbit_get, avr_regbit_set
#include "sim_time.h"
#include "uart_pty.h"       // for uart_pty
#include <cstdint>
#include <cstdlib>   // for exit, free
#include <cstring>    // for memcpy, NULL
#include <fstream>		// IWYU pragma: keep
#include <iomanip>          // for operator<<, setw
#include <iostream>
#include <memory>
#include <typeinfo>         // for type_info
#include <unistd.h>         // for usleep

namespace Boards {
	using string = std::string;

	Board::Board(const Wirings::Wiring &wiring,uint32_t uiFreqHz):Scriptable("Board"),m_wiring(wiring),m_uiFreq(uiFreqHz)
	{
		RegisterActionAndMenu("Quit", "Sends the quit signal to the AVR",ScriptAction::Quit);
		RegisterActionAndMenu("Reset","Resets the board by resetting the AVR.", ScriptAction::Reset);
		RegisterActionAndMenu("Pause","Pauses the simulated AVR execution.", ScriptAction::Pause);
		RegisterActionAndMenu("Resume","Resumes simulated AVR execution.", ScriptAction::Unpause);
		RegisterAction("WaitMs","Waits the specified number of milliseconds (in AVR-clock time)", ScriptAction::Wait,{ArgType::Int});

		RegisterKeyHandler('r', "Resets the AVR/board");
		RegisterKeyHandler('z', "Pauses/resumes AVR execution");
		RegisterKeyHandler('q', "Shuts down the board and exits");

	}

	Board::~Board()
	{
		if (m_thread) std::cerr << "PROGRAMMING ERROR: " << m_strBoard << " THREAD NOT STOPPED BEFORE DESTRUCTION.\n";
	}

	void Board::AddSerialPty(uart_pty *UART, const char chrNum)
	{
		UART->Init(m_pAVR);
		UART->Connect(chrNum);
	}

	bool Board::TryConnect(avr_irq_t *src, PinNames::Pin ePin)
	{
		if (m_wiring.IsPin(ePin))
		{
			avr_connect_irq(src, m_wiring.DIRQLU(m_pAVR,ePin));
			return true;
		}
		return _PinNotConnectedMsg(ePin);
	}

	bool Board::TryConnect(PinNames::Pin ePin, BasePeripheral* hw, unsigned int eDest)
	{
		if (m_wiring.IsPin(ePin))
		{
			hw->ConnectFrom(m_wiring.DIRQLU(m_pAVR,ePin),eDest);
			return true;
		}
		else
		{
			return _PinNotConnectedMsg(ePin);
		}
	};

	bool Board::TryConnect(BasePeripheral* hw, unsigned int eDest,PinNames::Pin ePin)
	{
		if (m_wiring.IsPin(ePin))
		{
			hw->ConnectTo(eDest,m_wiring.DIRQLU(m_pAVR,ePin));
			return true;
		}
		else
		{
			return _PinNotConnectedMsg(ePin);
		}
	};

	avr_irq_t* Board::GetDIRQ(PinNames::Pin ePin)
	{
		if (m_wiring.IsPin(ePin))
		{
			return m_wiring.DIRQLU(m_pAVR,ePin);
		}
		return nullptr;
	}

	Board::MCUPin Board::GetPinNumber(PinNames::Pin ePin)
	{
		if (m_wiring.IsPin(ePin))
		{
			return m_wiring.GetPin(ePin);
		}
		return -1;
	}

	avr_irq_t* Board::GetPWMIRQ(PinNames::Pin ePin)
	{
		if (m_wiring.IsPin(ePin))
		{
			return m_wiring.DPWMLU(m_pAVR,ePin);
		}
		return nullptr;
	}

	bool Board::_PinNotConnectedMsg(PinNames::Pin ePin)
	{
		std:: cout << "Requested connection w/ Digital pin " << ePin << " on " << m_strBoard << ", but it is not defined!\n";;
		return false;
	}

	void Board::CreateAVR()
	{
		m_pAVR = avr_make_mcu_by_name(m_wiring.GetMCUName().c_str());

		if (!m_pAVR)
		{
			std::cerr << "FATAL: Failed to create board " << m_wiring.GetMCUName() << '\n';
			exit(1);
		}
		m_pAVR->custom.init = [](avr_t *p, void *param){auto *board =   static_cast<Board*>(param); board->_OnAVRInit();};
		m_pAVR->custom.deinit = [](avr_t *p, void *param){auto *board = static_cast<Board*>(param); board->_OnAVRDeinit();};
		m_pAVR->custom.data = this;
		avr_init(m_pAVR);
		m_EEPROM.Load(m_pAVR,GetStorageFileName("eeprom").c_str());
	}

	void Board::CreateBoard(const string &strFW, uint8_t uiV,  bool bGDB, uint32_t uiVCDRate, const string &strBoot)
	{
		CreateAVR();
		if (!strFW.empty())
		{
			m_FWBase = LoadFirmware(strFW);
			m_pAVR->pc = m_FWBase;
		}
		else
		{
			std::cout << "NOTE: No firmware load requested, Executing purely from flash memory: " << m_strBoard << '\n';
		}

		if (!strBoot.empty())
		{
			m_bootBase = LoadFirmware(strBoot);
			m_pAVR->reset_pc = m_bootBase;
		}
		string strVCD = GetStorageFileName("VCD");
		strVCD.replace(strVCD.end()-3,strVCD.end(), "vcd");
		std::cout << "Initialized VCD file " << strVCD << '\n';

		m_pAVR->frequency = m_uiFreq;
		m_pAVR->vcc = 5000;
		m_pAVR->aref = 0;
		m_pAVR->avcc = 5000;
		m_pAVR->log = 1 + uiV;

		TelemetryHost::GetHost().Init(m_pAVR, strVCD,uiVCDRate);

		// even if not setup at startup, activate gdb if crashing
		m_pAVR->gdb_port = 1234;

		// Enable full GDB support
		if (bGDB)
		{
			m_pAVR->state = cpu_Stopped;
			avr_gdb_init(m_pAVR);
		}

		SetupHardware();
	};

	void Board::AddUARTTrace(const char chrUART)
	{
			auto &TH = TelemetryHost::GetHost();
			avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUTPUT); //NOLINT - complaint is external macro
			avr_irq_t * dst = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_INPUT);//NOLINT - complaint is external macro
			avr_irq_t * xon = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUT_XON); //NOLINT - complaint is external macro
			avr_irq_t * xoff = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUT_XOFF); //NOLINT - complaint is external macro

			TH.AddTrace(src, string("UART")+chrUART, {TC::Serial},8);
			TH.AddTrace(dst, string("UART")+chrUART, {TC::Serial},8);
			TH.AddTrace(xon, string("UART")+chrUART, {TC::Serial});
			TH.AddTrace(xoff, string("UART")+chrUART, {TC::Serial});
	}

	void Board::StartAVR()
	{
		if (m_thread!=0)
		{
			std::cout << "Attempted to start an already running " << m_wiring.GetMCUName() << '\n';
			return;
		}
		auto fRunCB =[](void * param) { auto p = static_cast<Board*>(param); return p->RunAVR();};
		pthread_create(&m_thread, nullptr, fRunCB, this);
	}

	void Board::StopAVR()
	{
		std::cout << "Stopping " << m_strBoard << "_" << m_wiring.GetMCUName() << '\n';
		if (m_thread==0)
		{
			return;
		}
		m_bQuit = true;
		pthread_join(m_thread,nullptr);
		m_thread = 0;
		std::cout << "Done\n";
	}

	void Board::_OnAVRInit()
	{
		std::string strFlash = GetStorageFileName("flash");
		std::ifstream fsIn(strFlash, fsIn.binary | fsIn.ate);
		if (!fsIn.is_open() || fsIn.tellg() < m_pAVR->flashend) {
			std::cerr << "ERROR: Could not open flash file. Flash contents will NOT persist." << '\n';
		}
		else
		{
			fsIn.seekg(fsIn.beg);
			fsIn.read(reinterpret_cast<char*>(m_pAVR->flash), m_pAVR->flashend+1); //NOLINT maybe if fstream supported unsigned chars...
			std::cout << strFlash << ": Read " << fsIn.gcount() << " bytes.\n";
			if (fsIn.fail() || fsIn.gcount() != m_pAVR->flashend+1)
			{
				std::cerr << "Unable to load flash memory.\n";
				exit(1);
			}
		}
		// NB: EEPROM happens later, because the AVR is not ready yet right now.
		fsIn.close();
		OnAVRInit();
	}

	void Board::_OnAVRDeinit()
	{
		std::ofstream fsOut(GetStorageFileName("flash"),fsOut.binary | fsOut.out | fsOut.trunc);
		if (!fsOut.is_open())
		{
			std::cerr << "Could not open flash file for writing\n";
		}
		else
		{
			fsOut.write(reinterpret_cast<char*>(m_pAVR->flash),m_pAVR->flashend+1); //NOLINT maybe if fstream supported unsigned chars...
			if ( fsOut.fail() || fsOut.tellp() != m_pAVR->flashend+1) {
				std::cerr <<  "Unable to write flash memory for " << m_strBoard << '\n';
			}
			else
			{
				std::cout << "Wrote " << fsOut.tellp() << " bytes of flash\n";
			}
			fsOut.close();
		}
		m_EEPROM.Save();
		OnAVRDeinit();
	}

	void Board::OnKeyPress(const Key& key)
	{
		switch (key)
		{
			case 'z':
			{
				m_bPaused ^= true;
				std::cout <<  "Pause: " << m_bPaused << '\n';
				auto tWall = avr_get_time_stamp(m_pAVR);
				auto tSim = avr_cycles_to_nsec(m_pAVR, m_pAVR->cycle);
				if (tWall<tSim)
				{
					std::cout << "Sim is ahead by" << std::to_string((tSim - tWall)/1000) << "us!\n";
				}
				else
				{
					std::cout << "Sim is behind by" << std::to_string((tWall - tSim)/1000) << "us!\n";
				}

			}
			break;
			case 'q':
				SetQuitFlag();
			break;
			case 'r':
				std::cout << "RESET/KILL\n";
				// RESET BUTTON
				SetResetFlag();
			break;
		}
	}

	avr_flashaddr_t Board::LoadFirmware(const string &strFW)
	{
		uint32_t uiFWSize = 0, uiFWStart = 0;
		if (strFW.size()>4)
		{
			if (0==strFW.compare(strFW.size()-4, 4, ".hex"))
			{
				gsl::unique_ptr<uint8_t> puiBytes {read_ihex_file(strFW.c_str(),&uiFWSize, &uiFWStart) };
				if (!puiBytes)
				{
					std::cout << "WARN: Could not load " << strFW << ". MCU will execute existing flash." << '\n';
				}
				else
				{
					std::cout << "Loaded "  << uiFWSize << " bytes from HEX file: " << strFW << '\n';
					gsl::span<uint8_t> flash {m_pAVR->flash, m_pAVR->flashend};
					memcpy(flash.begin() + uiFWStart, puiBytes.get(), uiFWSize);
				}
				m_pAVR->codeend = m_pAVR->flashend;
				return uiFWStart;
			}
			else if(0==strFW.compare(strFW.size()-4, 4, ".afx") ||
					0==strFW.compare(strFW.size()-4, 4, ".elf"))
			{
				elf_firmware_t fw = {};
				elf_read_firmware(strFW.c_str(), &fw);
				avr_load_firmware(m_pAVR, &fw);
				std::cout << "Loaded "  << fw.flashsize << " bytes from ELF file: " << strFW << '\n';
				return fw.flashbase;
			}
		}
		return 0;
	}

	void Board::DisableInterruptLevelPoll(uint8_t uiNumIntLins)
	{
		for (int i=0; i<uiNumIntLins; i++)
		{
			avr_extint_set_strict_lvl_trig(m_pAVR,i,false);
		}

	}

	IScriptable::LineStatus Board::ProcessAction(unsigned int ID, const std::vector<std::string> &vArgs)
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
				{
					if (--m_uiWtCycleCount==0)
					{
						return LineStatus::Finished;
					}
					else
					{
						return LineStatus::Waiting;
					}
				}
				else
				{
					m_uiWtCycleCount = (m_uiFreq/1000)*stoi(vArgs.at(0));
					return LineStatus::Waiting;
				}
				break;
			case Pause:
				std::cout << "Pause\n";
				m_bPaused.store(true);
				return LineStatus::Finished;
			case Unpause:
				m_bPaused.store(false);
				return LineStatus::Finished;
		}
		return LineStatus::Unhandled;
	}


	void* Board::RunAVR()
	{
		avr_regbit_t MCUSR = m_pAVR->reset_flags.porf;
		MCUSR.mask =0xFF;
		MCUSR.bit = 0;
		std::cout << "Starting " << m_wiring.GetMCUName() << " execution...\n";
		int state = cpu_Running;
		while ((state != cpu_Done) && (state != cpu_Crashed) && !m_bQuit){
			// Check the timing every 10k cycles, ~10 ms
			if (m_bCorrectSkew && m_pAVR->cycle%500==0)
			{
				auto tWall = avr_get_time_stamp(m_pAVR);
				auto tSim = avr_cycles_to_nsec(m_pAVR, m_pAVR->cycle);
				if (tWall<tSim)
				{
					auto tDiff = tSim - tWall;
					if (tDiff>200000) usleep(tDiff/1000);
					//std::cout << "Sim is ahead by" << std::to_string(tSim - tWall) << "ns!\n";
				}
			}
			if (m_bIsPrimary) // Only one board should be scripting.
			{
				ScriptHost::DispatchMenuCB();
				KeyController::GetController().OnAVRCycle(); // Handle/dispatch any pressed keys.
			}
			if (m_bIsPrimary && ScriptHost::IsInitialized())
			{
				ScriptHost::OnAVRCycle();
			}
			if (m_bPaused)
			{
				usleep(100000);
				continue;
			}
			int8_t uiMCUSR = avr_regbit_get(m_pAVR,MCUSR);
			if (uiMCUSR != m_uiLastMCUSR)
			{
				std::cout << "MCUSR: " << std::setw(2) << std::hex << (m_uiLastMCUSR = uiMCUSR) << '\n';
				if (uiMCUSR) // only run on change and not changed to 0
				{
					OnAVRReset();
				}
			}
			OnAVRCycle();

			if (m_bReset)
			{
				m_bReset = false;
				avr_reset(m_pAVR);
				avr_regbit_set(m_pAVR, m_pAVR->reset_flags.extrf);
			}
			state = avr_run(m_pAVR);
		}
		std::cout << m_wiring.GetMCUName() << "finished (" << state << ").\n";
		avr_terminate(m_pAVR);
		return nullptr;
	};


	std::string Board::GetStorageFileName(const std::string &strType)
	{
		std::string strFN {CXXDemangle(typeid(*this).name())};//= m_strBoard;
		//strFN.append("_").append(m_wiring.GetMCUName()).append("_").append(strType).append(".bin");
		strFN.append("_").append(strType).append(".bin");
#ifdef TEST_MODE // Creates special files in test mode that don't clobber your existing stuff.
			strFN.append("_test");
#endif
		return strFN;
	}
}; // namespace Boards
