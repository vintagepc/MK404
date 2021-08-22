/*
	w25x20cl.h - An SPI flash emulator for the Einsy external language flash

	Copyright 2020 leptun <https://github.com/leptun/>

	Rewritten for C++ in 2020 by VintagePC <https://github.com/vintagepc/>

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

#include "IScriptable.h"
#include "SPIPeripheral.h"  // for SPIPeripheral
#include "Scriptable.h"
#include "gsl-lite.hpp"
#include "sim_irq.h"        // for avr_irq_t
#include <cstdint>         // for uint8_t, uint32_t, uint64_t
#include <string>
#include <vector>

#define W25X20CL_TOTAL_SIZE 262144
#define W25X20CL_PAGE_SIZE 256U
#define W25X20CL_SECTOR_SIZE 4096
#define W25X20CL_BLOCK32_SIZE 32768
#define W25X20CL_BLOCK64_SIZE 65536

class w25x20cl:public SPIPeripheral, public Scriptable
{
	public:
		#define IRQPAIRS \
		_IRQ(SPI_BYTE_IN,       "8<w25x20cl.byte_in") \
		_IRQ(SPI_BYTE_OUT,  	"8>w25x20cl.byte_out") \
		_IRQ(SPI_CSEL,          "1<w25x20cl.cs_in")
		#include "IRQHelper.h"

	w25x20cl();

	// Destructor. Closes flash file.
	~w25x20cl() override;

	// Initializes an SPI flash on "avr" with a CSEL irq "irqCS"
	void Init(struct avr_t * avr, avr_irq_t *irqCS);

	// Loads the flash contents from file. (creates "path" if it does not exit)
	void Load(const std::string &path);

	// Loads from an array
	void Load(const gsl::span<uint8_t> data, uint32_t uiOffset = 0);

	// Reloads the current file.
	void Load();

	// Saves the SPI flash contents back out to file. (Does not close it in case you want to save multiple times)
	void Save();

	protected:

	#ifdef TEST_MODE
		friend void Test_w25x20cl_errors();
	#endif

		Scriptable::LineStatus ProcessAction (unsigned int iAct, const std::vector<std::string> &vArgs) override;

		enum w25x20cl_states{
			STATE_IDLE = 0, //when CS is HIGH
			STATE_LOADING,
			STATE_RUNNING,
		};

		enum Actions
		{
			ActLoad,
			ActSave,
			ActClear,
			ActFill
		};

		uint8_t OnSPIIn(avr_irq_t *irq, uint32_t value) override;
        void OnCSELIn(avr_irq_t *irq, uint32_t value) override;

		uint8_t _m_flash[W25X20CL_TOTAL_SIZE] = {0xFF};
		uint8_t _m_pageBuffer[W25X20CL_PAGE_SIZE] = {0xFF};
		uint8_t _m_cmdIn[5] = {0};
		gsl::span<uint8_t> m_flash {_m_flash};
		gsl::span<uint8_t> m_pageBuffer {_m_pageBuffer};
		gsl::span<uint8_t> m_cmdIn {_m_cmdIn};
		uint8_t m_rxCnt = 0;
		uint8_t m_cmdOut = 0;
		uint8_t m_command = 0;
		uint32_t m_address = 0;
		uint64_t m_UID = 0xDEADBEEFDEADBEEF;
		union
		{
			uint8_t byte;
			struct
			{
				uint8_t BUSY :1;
				uint8_t WEL :1;
				uint8_t BP :2;
				uint8_t RES4 :1;
				uint8_t TB :1;
				uint8_t RES6 :1;
				uint8_t SRP :1;
			} bits;
		} m_status_register {.byte = 0};
		w25x20cl_states m_state = STATE_IDLE;
		std::string m_filepath;

};
