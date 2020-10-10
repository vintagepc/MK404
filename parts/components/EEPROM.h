/*
	Einsy_EEPROM.h - helper to persist the AVR eeprom (and let us poke about in its internals...)

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


#pragma once

#include "BasePeripheral.h"  // for BasePeripheral
#include "IScriptable.h"     // for ArgType, ArgType::Int, IScriptable::Line...
#include "Scriptable.h"      // for Scriptable
#include <cstdint>          // for uint16_t, uint8_t
#include <string>            // for string
#include <vector>            // for vector

class EEPROM: public BasePeripheral, public Scriptable {
	public:

	EEPROM();
	// Loads EEPROM from a file or initializes the file for the first time.
	EEPROM(struct avr_t * avr, const std::string &strFile);

	// Loads the given file.
	void Load(struct avr_t * avr, const std::string &strFile);

	// Reloads last file:
	void Load();

	// Saves EEPROM to the file
	void Save();

	// Clears the contents to 0xFF
	void Clear();

	// Pokes something into the EEPROM.
	void Poke(uint16_t address,	uint8_t value);

	// Peeks at a value in the EEPROM.
	uint8_t Peek(uint16_t address);

	protected:
		LineStatus ProcessAction(unsigned int uiAct, const std::vector<std::string> &vArgs) override;


	private:
		std::string m_strFile;
		uint16_t m_uiSize = 4096;
		enum Actions {
			ActPoke,
			ActSave,
			ActLoad,
			ActClear,
			ActPeekVerify
		};

};
