/*
	GCodeSniffer.h - utility for sniffing GCODE from a serial connection.
	Its main purpose is for sniffing T-codes from the MMU. But it might have use elsewhere...

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
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t
#include <cstdint>          // for uint32_t
#include <string>            // for string

class GCodeSniffer : public BasePeripheral
{
	public:
		#define IRQPAIRS _IRQ(BYTE_IN,"8<logger.in") _IRQ(CODEVAL_OUT, "8>val_out")
		#include "IRQHelper.h"

		// Creates a logger that sniffs for
		explicit GCodeSniffer(unsigned char chrSniff):m_chrCode(chrSniff){};

		// Shuts down the logger/closes file.
		~GCodeSniffer() = default;

		// Registers with SimAVR.
		void Init(avr_t *avr, unsigned char chrUART);

		inline std::string GetName(){return std::string("Sniffer");}

	private:

		void OnByteIn(avr_irq_t *irq, uint32_t value);

		unsigned char m_chrCode;
		std::string m_strLine;
		bool m_bNewLine = false, m_bCapture = false;
		char m_chrUART = '0';

};
