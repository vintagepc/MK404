/*
	SerialLineMonitor.h - Scripting extension for the UART output.

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
#include "IScriptable.h"     // for ArgType, ArgType::String, IScriptable::L...
#include "Scriptable.h"      // for Scriptable
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t
#include <cstdint>          // for uint32_t
#include <string>            // for string, basic_string
#include <vector>            // for vector

class SerialLineMonitor : public BasePeripheral,public Scriptable
{
	public:
		#define IRQPAIRS _IRQ(BYTE_IN,"8<monitor.in") _IRQ(BYTE_OUT,"8>monitor.out")
		#include "IRQHelper.h"

		// Creates a logger that sniffs for
		explicit SerialLineMonitor(const std::string &strName);

		// Shuts down
		~SerialLineMonitor() override = default;

		// Registers with SimAVR.
		void Init(avr_t *avr, char chrUART);
	protected:
		LineStatus ProcessAction(unsigned int ID, const std::vector<std::string> &args) override;

	private:
		enum matchType
		{
			None = 0,
			Full,
			Contains,
			MustBe
		};
		void OnByteIn(avr_irq_t *irq, uint32_t value);
		void OnXOnIn(avr_irq_t *irq, uint32_t value);
		void OnXOffIn(avr_irq_t *irq, uint32_t value);
		void OnNewLine();

		LineStatus SendChar();

		matchType m_type = None;


		std::string m_strLine;
		std::string m_strMatch;
		std::string m_strGCode;

		std::string::iterator  m_itGCode;

		char m_chrUART = '0';
		bool m_bMatched = false;
		unsigned int m_iLineCt = 0;
		bool m_bXOn = true;
		enum ScriptAction {
			WaitForLine,
			WaitForContains,
			NextLineMustBe,
			SendGCode
		};

};
