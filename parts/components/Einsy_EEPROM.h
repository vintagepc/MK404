/*
	Einsy_EEPROM.h - helper to persist the AVR eeprom (and let us poke about in its internals...)

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


#ifndef __EINSY_EEPROM_H__
#define __EINSY_EEPROM_H__

#include "stdlib.h"
#include "unistd.h"
#include "BasePeripheral.h"

class Einsy_EEPROM: public BasePeripheral {
	public:

	Einsy_EEPROM(){};
	// Loads EEPROM from a file or initializes the file for the first time.
	Einsy_EEPROM(struct avr_t * avr, const char* path)
	{ Load(avr, path);};

	void Load(struct avr_t * avr, const char* path);
	// Saves EEPROM to the file
	void Save();

	// Pokes something into the EEPROM.
	void Poke(uint16_t address,	uint8_t value);

	// Peeks at a value in the EEPROM.
	uint8_t Peek(uint16_t address);


	private:
		char m_strPath[1024];
		int m_fdEEPROM = 0;
		uint16_t m_uiSize = 4096;

};

#endif
