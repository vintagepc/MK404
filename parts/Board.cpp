/*
	Board.cpp - Base class for a "board"
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

#include <Board.h>
#include <sim_hex.h>
#include <sim_elf.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

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
}

void Board::CreateBoard()
{
	CreateAVR();
	if (!m_strFW.empty())
	{
		m_FWBase = LoadFirmware(m_strFW);
		m_pAVR->pc = m_FWBase;
	}
	else
		printf("NOTE: No firmware load requested, %s executing purely from flash memory.\n", m_strBoard.c_str());

	if (!m_strBoot.empty())
	{
		m_bootBase = LoadFirmware(m_strBoot);
		m_pAVR->reset_pc = m_bootBase;
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
	std::string strFlash = GetStorageFileName("flash"),
		strEEPROM = GetStorageFileName("eeprom");

	m_fdFlash = open(strFlash.c_str(), O_RDWR|O_CREAT, 0644);
	if (m_fdFlash < 0) {
		perror(strFlash.c_str());
		fprintf(stderr,"ERROR: Could not open flash file. Flash contents will NOT persist.\n");
	}
	else
	{
		// resize and map the file the file
		(void)ftruncate(m_fdFlash, m_pAVR->flashend + 1);
		ssize_t r = read(m_fdFlash, m_pAVR->flash, m_pAVR->flashend + 1);
		if (r != m_pAVR->flashend + 1) {
			fprintf(stderr, "unable to load flash memory\n");
			perror(strFlash.c_str());
			exit(1);
		}
	}
	m_EEPROM.Load(m_pAVR,strEEPROM.c_str());
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
			printf("Loaded %u bytes from HEX file: %s\n",uiFWSize, strFW.c_str());
			memcpy(m_pAVR->flash + uiFWStart, puiBytes, uiFWSize);
			free(puiBytes);
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
