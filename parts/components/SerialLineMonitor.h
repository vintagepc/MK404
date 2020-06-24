/*
	SerialLineMonitor.h - Scripting extension for the UART output.

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

#pragma once

#include <stdint.h>          // for uint32_t
#include <string>            // for string, basic_string
#include <vector>            // for vector
#include "BasePeripheral.h"  // for BasePeripheral
#include "IScriptable.h"     // for ArgType, ArgType::String, IScriptable::L...
#include "Scriptable.h"      // for Scriptable
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t

using namespace std;

class SerialLineMonitor : public BasePeripheral,public Scriptable
{
	public:
		#define IRQPAIRS _IRQ(BYTE_IN,"8<monitor.in") _IRQ(BYTE_OUT,"8>monitor.out")
		#include "IRQHelper.h"

		// Creates a logger that sniffs for
		SerialLineMonitor(string strName):Scriptable(strName)
		{
			RegisterAction("WaitForLine","Waits for the provided line to appear on the serial output.",WaitForLine, {ArgType::String});
			RegisterAction("WaitForLineContains","Waits for the serial output to contain a line with the given string.",WaitForContains,{ArgType::String});
			RegisterAction("SendGCode","Sends the specified string as G-Code.",SendGCode,{ArgType::String});
			m_strLine.reserve(100);
		};

		// Shuts down
		~SerialLineMonitor(){};

		// Registers with SimAVR.
		void Init(avr_t *avr, char chrUART);
	protected:
		LineStatus ProcessAction(unsigned int ID, const vector<string> &args) override;

	private:
		enum matchType
		{
			None = 0,
			Full,
			Contains
		};
		void OnByteIn(avr_irq_t *irq, uint32_t value);
		void OnXOnIn(avr_irq_t *irq, uint32_t value);
		void OnXOffIn(avr_irq_t *irq, uint32_t value);
		void OnNewLine();

		LineStatus SendChar();

		matchType m_type = None;


		string m_strLine;
		string m_strMatch;
		string m_strGCode;

		std::string::iterator  m_itGCode;

		char m_chrUART = '0';
		bool m_bMatched = false;
		bool m_bXOn = false;
		enum ScriptAction {
			WaitForLine,
			WaitForContains,
			SendGCode
		};

};
