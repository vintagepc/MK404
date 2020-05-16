/*
	Einsy_EEPROM.cpp - helper to persist the AVR eeprom (and let us poke about in its internals...)

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
#include "Einsy_EEPROM.h"
#include <avr_eeprom.h>


Einsy_EEPROM::Einsy_EEPROM(struct avr_t *avr, const char* path)
{
	m_pAVR = avr;
	strncpy(m_strPath, path, sizeof(m_strPath));

	m_fdEEPROM = open(m_strPath, O_RDWR | O_CREAT, 0644);
	if (m_fdEEPROM < 0) {
		perror(m_strPath);
		exit(1);
	}
	printf("Loading %u bytes of EEPROM\n", m_uiSize);
	
	avr_eeprom_desc_t io {ee:(uint8_t*)malloc(m_uiSize),offset:0, size:m_uiSize};

	(void)ftruncate(m_fdEEPROM, m_uiSize);
	ssize_t r = read(m_fdEEPROM, io.ee, m_uiSize);
	printf("Read %d bytes\n",(int)r);
	if (r !=  io.size) {
		fprintf(stderr, "unable to load EEPROM\n");
		perror(m_strPath);
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

void Einsy_EEPROM::Save()
{
	// Write out the EEPROM contents:
	lseek(m_fdEEPROM, SEEK_SET, 0);

	avr_eeprom_desc_t io {ee:nullptr, offset:0, size:m_uiSize};
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_GET,&io); // Should net a pointer to eeprom[0]
	ssize_t r = write(m_fdEEPROM, io.ee, m_uiSize);
	printf("Wrote %zd bytes of EEPROM to %s\n",r, m_strPath);
	if (r != m_uiSize) {
		fprintf(stderr, "unable to write EEPROM memory\n");
		perror(m_strPath);
	}
	close(m_fdEEPROM);
}


void Einsy_EEPROM::Poke(uint16_t address, uint8_t value)
{
	avr_eeprom_desc_t io {ee: &value, offset:address, size:1};
	assert(address<m_uiSize);
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_SET,&io);
}

uint8_t Einsy_EEPROM::Peek(uint16_t address)
{
	uint8_t uiRet = 0;
	avr_eeprom_desc_t io {ee: &uiRet, offset:address, size:1};
	assert(address<m_uiSize);
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_GET,&io);
	return uiRet;
}

