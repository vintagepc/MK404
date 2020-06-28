/*
	TelemetryHost.h - Wrangler for VCD traces and scripted telemetry

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

#include <sim_vcd_file.h>
#include "BasePeripheral.h"
#include <string>
#include <vector>
#include <map>
#include <string.h>

using namespace std;

// A collection of "categories". A VCD object can
// list one or more of these when registering a trace.
// Only items in categries specified by a command line
// argument (or explicitly named) will be logged.
enum class TelCategory
{
	// Type
	Heater,
	Stepper,
	Display,
	Thermistor,
	Power,
	InputPin,
	Fan,
	OutputPin, // Output/input relative to board.
	// Connection
	Serial,
	SPI,
	I2C,
	ADC,
	PWM,
	Misc
};
typedef const vector<TelCategory>& TelCats;
using TC = TelCategory;

class TelemetryHost: public BasePeripheral
{
	public:

	#define IRQPAIRS
	#include "IRQHelper.h"



	inline static TelemetryHost* GetHost()
	{
		if (m_pHost == nullptr)
		{
			printf("TelemetryHost::Init\n");
		}
		return m_pHost;
	}

	// Inits the VCD file at the specified rate (in us)
	void Init(avr_t *pAVR, const string &strVCDFile, uint32_t uiRateUs = 100)
	{
		_Init(pAVR, this);
		avr_vcd_init(m_pAVR,strVCDFile.c_str(),&m_trace,uiRateUs);
	}

	inline void StartTrace()
	{
		avr_vcd_start(&m_trace);
	}

	inline void StopTrace()
	{
		avr_vcd_stop(&m_trace);
	}

	void SetCategories(const vector<string> &vsCats);

	void AddTrace(avr_irq_t *pIRQ, string strName, TelCats vCats, uint8_t uiBits = 1);

	void Shutdown()
	{
		StopTrace();
	}

	private:
		TelemetryHost()
		{
			if (m_pHost !=nullptr)
			{
				fprintf(stderr, "ERROR - duplicate initialization of telemetry host!\n");
				exit(1);
			}
			memset(&m_trace, 0, sizeof(m_trace));
		}

		avr_vcd_t m_trace;

		vector<TelCategory> m_VLoglst;
		vector<string> m_vsNames;

		static TelemetryHost *m_pHost;

		const map<string,TC> m_mCats = {
			make_pair("Heater", TC::Heater),
			make_pair("Stepper",TC::Stepper),
			make_pair("Display",TC::Display),
			make_pair("Thermistor",TC::Thermistor),
			make_pair("Power",TC::Power),
			make_pair("InputPin",TC::InputPin),
			make_pair("Fan",TC::Fan),
			make_pair("OutputPin",TC::OutputPin),
			make_pair("Serial",TC::Serial),
			make_pair("SPI",TC::SPI),
			make_pair("I2C",TC::I2C),
			make_pair("ADC",TC::ADC),
			make_pair("PWM",TC::PWM),
			make_pair("Misc",TC::Misc)
		};

};
