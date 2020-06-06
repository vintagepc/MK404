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
	avr_init(m_pAVR);
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
			printf("Loaded %u bytes from ELF file: %s\n",uiFWSize, strFW.c_str());
			return fw.flashbase;
		}
	}
	return 0;
}