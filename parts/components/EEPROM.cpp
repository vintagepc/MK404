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
#include "avr_eeprom.h"  // for avr_eeprom_desc_t, AVR_IOCTL_EEPROM_GET, AVR...
#include "gsl-lite.hpp"
#include "sim_avr.h"     // for avr_t
#include "sim_io.h"      // for avr_ioctl
#include "unistd.h"      // for close, ftruncate, lseek, read, write
#include <cstdlib>      // for malloc, exit, free, size_t
#include <fstream>       // for open, O_CREAT, O_RDWR, SEEK_SET
#include <iostream>       // for perror, printf, fprintf, stderr
#include <sys/types.h>   // for ssize_t

using std::ifstream;
using std::ofstream;

void EEPROM::Load(struct avr_t *avr, const string &strFile)
{
	m_strFile = strFile;
	m_pAVR = avr;
	Load();
}

void EEPROM::Load()
{
	m_uiSize = m_pAVR->e2end + 1;

    ifstream fsIn(strFile, fsIn.binary | fsIn.ate);
	if (!fsIn.is_open() || fsIn.tellg() < m_pAVR->e2end) {
		cerr << "ERROR: Could not open flash file. Flash contents were NOT restored" << '\n';
	}
	else
	{
		cout << "Loading " << m_uiSize  <<" bytes of EEPROM\n";
		vector<uint8_t> vEE;
		vEE.resize(m_uiSize,0);
		avr_eeprom_desc_t io {.ee= vEE.data(), .offset = 0, .size = m_uiSize};
		fsIn.seekg(fsIn.beg);
		fsIn.read(reinterpret_cast<char*>(vEE.data()), m_uiSize); //NOLINT maybe if fstream supported unsigned chars...
		cout << "Read " << fsIn.tellg() << " bytes\n";
		if (fsIn.fail() || fsIn.gcount() != m_uiSize) {
			cerr << "Unable to load EEPROM\n";
			exit(1);
		}
		bool bEmpty = true;
		for (auto &b : vEE)
		{
			bEmpty &= b==0;
		}
		if (!bEmpty) // If the file was newly created (all null) this leaves the internal eeprom as full of 0xFFs.
			avr_ioctl(m_pAVR, AVR_IOCTL_EEPROM_SET,&io); //NOLINT- complaint is external macro
	}
	fsIn.close();
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
	ofstream fsOut(m_strFile, fsOut.binary | fsOut.out | fsOut.trunc);
	if (!fsOut.is_open())
	{
		cerr << "Failed to open EEPROM output file\n";
		return;
	}
	vector<uint8_t> vEE;
	vEE.resize(m_uiSize,0);
	avr_eeprom_desc_t io {.ee= vEE.data(), .offset = 0, .size = m_uiSize};
	//NOLINTNEXTLINE - complaint is external macro
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_GET,&io); // Should net a pointer to eeprom[0]
	fsOut.write(reinterpret_cast<char*>(vEE.data()),m_uiSize); //NOLINT maybe if fstream supported unsigned chars...
	cout << "Wrote "<< fsOut.tellp() <<" bytes of EEPROM to " << m_strFile <<'\n';
	if (fsOut.tellp() != m_uiSize) {
		cerr << "Unable to write EEPROM memory\n";
	}
	fsOut.close();
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
	Expects(address<m_uiSize);
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_SET,&io); //NOLINT - complaint is external macro
}

uint8_t EEPROM::Peek(uint16_t address)
{
	uint8_t uiRet = 0;
	avr_eeprom_desc_t io {.ee = &uiRet, .offset = address, .size = 1};
	Expects(address<m_uiSize);
	avr_ioctl(m_pAVR,AVR_IOCTL_EEPROM_GET,&io); //NOLINT - complaint is external macro
	return uiRet;
}
