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

#include <Board.h>
#include <fcntl.h>    // for open, O_CREAT, O_RDWR, SEEK_SET
#include "sim_elf.h"  // for avr_load_firmware, elf_firmware_t, elf_read_fir...
#include "sim_gdb.h"  // for avr_gdb_init
#include "sim_hex.h"  // for read_ihex_file
#include <stdlib.h>   // for exit, free
#include <unistd.h>   // for close, ftruncate, lseek, read, write, ssize_t
#include <cstring>    // for memcpy, NULL
#include "sim_io.h"         // for avr_io_getirq
#include "avr_uart.h"
#include "TelemetryHost.h"

using namespace std;
using namespace Boards;

void Board::CreateAVR()
{
	m_pAVR = avr_make_mcu_by_name(m_wiring.GetMCUName().c_str());

	if (!m_pAVR)
	{
		fprintf(stderr, "FATAL: Failed to create board %s\n", m_wiring.GetMCUName().c_str());
		exit(1);
	}
	m_pAVR->custom.init = [](avr_t *p, void *param){Board *board = (Board*)param; board->_OnAVRInit();};
	m_pAVR->custom.deinit = [](avr_t *p, void *param){Board *board = (Board*)param; board->_OnAVRDeinit();};
	m_pAVR->custom.data = this;
	avr_init(m_pAVR);
	m_EEPROM.Load(m_pAVR,GetStorageFileName("eeprom").c_str());
}

void Board::CreateBoard(string strFW, uint8_t uiV,  bool bGDB, uint32_t uiVCDRate, string strBoot)
{
	CreateAVR();
	if (!strFW.empty())
	{
		m_FWBase = LoadFirmware(strFW);
		m_pAVR->pc = m_FWBase;
	}
	else
		printf("NOTE: No firmware load requested, %s executing purely from flash memory.\n", m_strBoard.c_str());

	if (!strBoot.empty())
	{
		m_bootBase = LoadFirmware(strBoot);
		m_pAVR->reset_pc = m_bootBase;
	}
	string strVCD = GetStorageFileName("VCD");
	strVCD.replace(strVCD.end()-3,strVCD.end(), "vcd");
	printf("Initialized VCD file %s\n",strVCD.c_str());

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
		printf("Attempted to start an already running %s\n", m_wiring.GetMCUName().c_str());
		return;
	}
	auto fRunCB =[](void * param) { Board* p = (Board*)param; return p->RunAVR();};
	pthread_create(&m_thread, NULL, fRunCB, this);
}

void Board::StopAVR()
{
	printf("Stopping %s_%s...\n", m_strBoard.c_str(), m_wiring.GetMCUName().c_str());
	if (m_thread==0)
		return;
	m_bQuit = true;
	pthread_join(m_thread,NULL);
	m_thread = 0;
	printf("Done\n");
}

void Board::_OnAVRInit()
{
	std::string strFlash = GetStorageFileName("flash");

	m_fdFlash = open(strFlash.c_str(), O_RDWR|O_CREAT, 0644);
	if (m_fdFlash < 0) {
		perror(strFlash.c_str());
		fprintf(stderr,"ERROR: Could not open flash file. Flash contents will NOT persist.\n");
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
			fprintf(stderr, "unable to load flash memory\n");
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
			fprintf(stderr, "unable to write %s flash memory\n",m_strBoard.c_str());
		}
		close(m_fdFlash);
		m_fdFlash = 0;
	}
	m_EEPROM.Save();
	OnAVRDeinit();
}

avr_flashaddr_t Board::LoadFirmware(string strFW)
{
	uint8_t *puiBytes = nullptr;
	uint32_t uiFWSize = 0, uiFWStart = 0;
	if (strFW.size()>4)
	{
		if (0==strFW.compare(strFW.size()-4, 4, ".hex"))
		{
			puiBytes = read_ihex_file(strFW.c_str(),&uiFWSize, &uiFWStart);
			if (!puiBytes)
				printf("WARN: Could not load %s. MCU will execute existing flash.\n", strFW.c_str());
			else
			{
				printf("Loaded %u bytes from HEX file: %s\n",uiFWSize, strFW.c_str());
				memcpy(m_pAVR->flash + uiFWStart, puiBytes, uiFWSize);
				free(puiBytes);
			}
			m_pAVR->codeend = m_pAVR->flashend;
			return uiFWStart;
		}
		else if(0==strFW.compare(strFW.size()-4, 4, ".afx") ||
				0==strFW.compare(strFW.size()-4, 4, ".elf"))
		{
			elf_firmware_t fw;
			elf_read_firmware(strFW.c_str(), &fw);
			avr_load_firmware(m_pAVR, &fw);
			printf("Loaded %u bytes from ELF file: %s\n",fw.flashsize, strFW.c_str());
			return fw.flashbase;
		}
	}
	return 0;
}
