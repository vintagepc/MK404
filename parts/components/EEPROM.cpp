/*
	EEPROM.cpp - helper to persist the AVR eeprom (and let us poke about in its internals...)

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

#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include <fcntl.h>
#include "string.h"
#include "EEPROM.h"
#include <avr_eeprom.h>


void EEPROM::Load(struct avr_t *avr, const string &strFile)
{
	m_pAVR = avr;
	m_uiSize = m_pAVR->e2end + 1;
	m_strFile = strFile;

	m_fdEEPROM = open(m_strFile.c_str(), O_RDWR | O_CREAT, 0644);
	if (m_fdEEPROM < 0) {
		perror(m_strFile.c_str());
		exit(1);
	}
	printf("Loading %u bytes of EEPROM\n", m_uiSize);
	avr_eeprom_desc_t io {ee:(uint8_t*)malloc(m_uiSize),offset:0, size:m_uiSize};

	(void)ftruncate(m_fdEEPROM, m_uiSize);
	ssize_t r = read(m_fdEEPROM, io.ee, m_uiSize);
	printf("Read %d bytes\n",(int)r);
	if (r !=  io.size) {
		fprintf(stderr, "unable to load EEPROM\n");
		perror(m_strFile.c_str());
		exit(1);
	}
	uint8_t bEmpty = 1;
	for (int i=0; i<io.size; i++)
	{
		bEmpty &= io.ee[i]==0;
	}
	if (!bEmpty) // If the file was newly created (all null) this leaves the internal eeprom as full of 0xFFs.
		avr_ioctl(m_pAVR, AVR_IOCTL_EEPROM_SET,&io);

    free(io.ee);
}

void EEPROM::Save()
{
	// Write out the EEPROM contents:
	lseek(m_fdEEPROM, SEEK_SET, 0);

	avr_eeprom_desc_t io {ee:(uint8_t*)malloc(m_uiSize), offset:0, size:m_uiSize};
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
			unsigned int uiAddr = stoi(vArgs.at(0));
			uint8_t uiVal = stoi(vArgs.at(1));
			if (uiAddr<0 || uiAddr>=m_uiSize)
			{
				printf("EEPROM: Error - Address %d is out of range [0 - %d]", uiAddr,m_uiSize);
				return LineStatus::Error;
			}
			else
			{
				Poke(uiAddr, uiVal);
				return LineStatus::Finished;
			}
	}
}


void EEPROM::Poke(uint16_t address, uint8_t value)
{
	avr_eeprom_desc_t io {ee: &value, offset:address, size:1};
	assert(address<m_uiSize);
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_SET,&io);
}

uint8_t EEPROM::Peek(uint16_t address)
{
	uint8_t uiRet = 0;
	avr_eeprom_desc_t io {ee: &uiRet, offset:address, size:1};
	assert(address<m_uiSize);
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_GET,&io);
	return uiRet;
}
