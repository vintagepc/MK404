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

#include "BasePeripheral.h"
#include "Scriptable.h"
#include <string>

using namespace std;

class SerialLineMonitor : public BasePeripheral,public Scriptable
{
	public:
		#define IRQPAIRS _IRQ(BYTE_IN,"8<monitor.in")
		#include "IRQHelper.h"

		// Creates a logger that sniffs for
		SerialLineMonitor(string strName):Scriptable(strName)
		{
			RegisterAction("WaitForLine","Waits for the provided line to appear on the serial output.",WaitForLine, {ArgType::String});
			RegisterAction("WaitForLineContains","Waits for the serial output to contain a line with the given string.",WaitForContains,{ArgType::String});
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
		void OnNewLine();

		matchType m_type = None;

		string m_strLine;
		string m_strMatch;

		char m_chrUART = '0';
		bool m_bMatched = false;
		enum ScriptAction {
			WaitForLine,
			WaitForContains
		};

};
