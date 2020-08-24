/*
	EEPROM.cpp - helper to persist the AVR eeprom (and let us poke about in its internals...)

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK404

	MK404is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK404is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK404  If not, see <http://www.gnu.org/licenses/>.
 */

#include "EEPROM.h"
#include <avr_eeprom.h>  // for avr_eeprom_desc_t, AVR_IOCTL_EEPROM_GET, AVR...
#include <fcntl.h>       // for open, O_CREAT, O_RDWR, SEEK_SET
#include <stdlib.h>      // for malloc, exit, free, size_t
#include <sys/types.h>   // for ssize_t
#include "assert.h"      // for assert
#include "sim_avr.h"     // for avr_t
#include "sim_io.h"      // for avr_ioctl
#include "stdio.h"       // for perror, printf, fprintf, stderr
#include "unistd.h"      // for close, ftruncate, lseek, read, write


void EEPROM::Load(struct avr_t *avr, const string &strFile)
{
	m_strFile = strFile;
	m_pAVR = avr;
	Load();
}

void EEPROM::Load()
{
	m_uiSize = m_pAVR->e2end + 1;

	m_fdEEPROM = open(m_strFile.c_str(), O_RDWR | O_CREAT, 0644);
	if (m_fdEEPROM < 0) {
		perror(m_strFile.c_str());
		exit(1);
	}
	printf("Loading %u bytes of EEPROM\n", m_uiSize);
	avr_eeprom_desc_t io {.ee= (uint8_t*)malloc(m_uiSize), .offset = 0, .size = m_uiSize};

	if (ftruncate(m_fdEEPROM, m_uiSize) < 0) {
		perror(m_strFile.c_str());
		exit(1);
	}
	ssize_t r = read(m_fdEEPROM, io.ee, m_uiSize);
	printf("Read %d bytes\n",(int)r);
	if (r !=  io.size) {
		fprintf(stderr, "unable to load EEPROM\n");
		perror(m_strFile.c_str());
		exit(1);
	}
	uint8_t bEmpty = 1;
	for (size_t i=0; i<io.size; i++)
	{
		bEmpty &= io.ee[i]==0;
	}
	if (!bEmpty) // If the file was newly created (all null) this leaves the internal eeprom as full of 0xFFs.
		avr_ioctl(m_pAVR, AVR_IOCTL_EEPROM_SET,&io);

    free(io.ee);
}

void EEPROM::Clear()
{
	std::vector<uint8_t> vE;
	vE.resize(m_uiSize,0xFF);
	avr_eeprom_desc_t io {.ee= vE.data(), .offset = 0, .size = m_uiSize};
	avr_ioctl(m_pAVR, AVR_IOCTL_EEPROM_SET,&io);
}

void EEPROM::Save()
{
	// Write out the EEPROM contents:
	lseek(m_fdEEPROM, SEEK_SET, 0);

	avr_eeprom_desc_t io {.ee= (uint8_t*)malloc(m_uiSize), .offset = 0, .size = m_uiSize};
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_GET,&io); // Should net a pointer to eeprom[0]

	ssize_t r = write(m_fdEEPROM, io.ee, m_uiSize);
	printf("Wrote %zd bytes of EEPROM to %s\n",r, m_strFile.c_str());
	if (r != m_uiSize) {
		fprintf(stderr, "unable to write EEPROM memory\n");
		perror(m_strFile.c_str());
	}
	close(m_fdEEPROM);
	free(io.ee);
}

Scriptable::LineStatus EEPROM::ProcessAction(unsigned int uiAct, const vector<string> &vArgs)
{
	switch (uiAct)
	{
		case ActPoke:
		{
			unsigned int uiAddr = stoi(vArgs.at(0));
			uint8_t uiVal = stoi(vArgs.at(1));
			if (uiAddr>=m_uiSize)
				return IssueLineError(string("Address ") + to_string(uiAddr) + " is out of range [0," + to_string(m_uiSize-1) + "]");
			else
			{
				Poke(uiAddr, uiVal);
				return LineStatus::Finished;
			}
		}
		break;
		case ActClear:
		{
			Clear();
			return LineStatus::Finished;
		}
		break;
		case ActSave:
		{
			Save();
			return LineStatus::Finished;
		}
		break;
		case ActLoad:
		{
			Load();
			return LineStatus::Finished;
		}
		break;
	}
	return LineStatus::Unhandled;
}


void EEPROM::Poke(uint16_t address, uint8_t value)
{
	avr_eeprom_desc_t io {.ee = &value, .offset = address, .size = 1};
	assert(address<m_uiSize);
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_SET,&io);
}

uint8_t EEPROM::Peek(uint16_t address)
{
	uint8_t uiRet = 0;
	avr_eeprom_desc_t io {.ee = &uiRet, .offset = address, .size = 1};
	assert(address<m_uiSize);
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_GET,&io);
	return uiRet;
}
