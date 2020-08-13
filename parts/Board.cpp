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
#include "TelemetryHost.h"
#include "avr_uart.h"
#include "gsl-lite.hpp"
#include "sim_elf.h"  // for avr_load_firmware, elf_firmware_t, elf_read_fir...
#include "sim_gdb.h"  // for avr_gdb_init
#include "sim_hex.h"  // for read_ihex_file
#include "sim_io.h"         // for avr_io_getirq
#include <cstdlib>   // for exit, free
#include <cstring>    // for memcpy, NULL
#include <fcntl.h>    // for open, O_CREAT, O_RDWR, SEEK_SET
#include <iostream>
#include <unistd.h>   // for close, ftruncate, lseek, read, write, ssize_t

namespace Boards {
	void Board::CreateAVR()
	{
		m_pAVR = avr_make_mcu_by_name(m_wiring.GetMCUName().c_str());

		if (!m_pAVR)
		{
			cerr << "FATAL: Failed to create board " << m_wiring.GetMCUName() << '\n';
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
			cout << "NOTE: No firmware load requested, Executing purely from flash memory: " << m_strBoard << '\n';
		}

		if (!strBoot.empty())
		{
			m_bootBase = LoadFirmware(strBoot);
			m_pAVR->reset_pc = m_bootBase;
		}
		string strVCD = GetStorageFileName("VCD");
		strVCD.replace(strVCD.end()-3,strVCD.end(), "vcd");
		cout << "Initialized VCD file " << strVCD << '\n';

		m_pAVR->frequency = m_uiFreq;
		m_pAVR->vcc = 5000;
		m_pAVR->aref = 0;
		m_pAVR->avcc = 5000;
		m_pAVR->log = 1 + uiV;

		TelemetryHost::GetHost()->Init(m_pAVR, strVCD,uiVCDRate);

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
			auto pTH = TelemetryHost::GetHost();
			avr_irq_t * src = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUTPUT);
			avr_irq_t * dst = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_INPUT);
			avr_irq_t * xon = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUT_XON);
			avr_irq_t * xoff = avr_io_getirq(m_pAVR, AVR_IOCTL_UART_GETIRQ(chrUART), UART_IRQ_OUT_XOFF);

			pTH->AddTrace(src, string("UART")+chrUART, {TC::Serial},8);
			pTH->AddTrace(dst, string("UART")+chrUART, {TC::Serial},8);
			pTH->AddTrace(xon, string("UART")+chrUART, {TC::Serial});
			pTH->AddTrace(xoff, string("UART")+chrUART, {TC::Serial});
	}

	void Board::StartAVR()
	{
		if (m_thread!=0)
		{
			cout << "Attempted to start an already running " << m_wiring.GetMCUName() << '\n';
			return;
		}
		auto fRunCB =[](void * param) { auto p = static_cast<Board*>(param); return p->RunAVR();};
		pthread_create(&m_thread, nullptr, fRunCB, this);
	}

	void Board::StopAVR()
	{
		cout << "Stopping " << m_strBoard << "_" << m_wiring.GetMCUName() << '\n';
		if (m_thread==0)
			return;
		m_bQuit = true;
		pthread_join(m_thread,nullptr);
		m_thread = 0;
		cout << "Done\n";
	}

	void Board::_OnAVRInit()
	{
		std::string strFlash = GetStorageFileName("flash");

		m_fdFlash = open(strFlash.c_str(), O_RDWR|O_CREAT|O_CLOEXEC, 0644);
		if (m_fdFlash < 0) {
			perror(strFlash.c_str());
			cerr << "ERROR: Could not open flash file. Flash contents will NOT persist." << '\n';
		}
		else
		{
			// resize and map the file the file
			if (ftruncate(m_fdFlash, m_pAVR->flashend + 1) < 0) {
				perror(strFlash.c_str());
				exit(1);
			}
			ssize_t r = read(m_fdFlash, m_pAVR->flash, m_pAVR->flashend + 1);
			if (r != m_pAVR->flashend + 1) {
				cerr << "Unable to load flash memory" << '\n';
				perror(strFlash.c_str());
				exit(1);
			}
		}
		// NB: EEPROM happens later, because the AVR is not ready yet right now.
		OnAVRInit();
	}

	void Board::_OnAVRDeinit()
	{
		if (m_fdFlash>0)
		{
			lseek(m_fdFlash, SEEK_SET, 0);
			ssize_t r = write(m_fdFlash, m_pAVR->flash, m_pAVR->flashend + 1);
			if (r != m_pAVR->flashend + 1) {
				cerr <<  "Unable to write flash memory for " << m_strBoard << '\n';
			}
			close(m_fdFlash);
			m_fdFlash = 0;
		}
		m_EEPROM.Save();
		OnAVRDeinit();
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
					cout << "WARN: Could not load " << strFW << ". MCU will execute existing flash." << '\n';
				else
				{
					cout << "Loaded "  << uiFWSize << " bytes from HEX file: " << strFW << '\n';
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
				cout << "Loaded "  << fw.flashsize << " bytes from ELF file: " << strFW << '\n';
				return fw.flashbase;
			}
		}
		return 0;
	}
}; // namespace Boards
