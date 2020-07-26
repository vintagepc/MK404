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
#include "Scriptable.h"

using namespace std;

#define TCENTRIES \
	_TC(Display,"Display"),\
	_TC(Fan,"Fan"),\
	_TC(Heater,"Heater"),\
	_TC(InputPin,"InputPin"),\
	_TC(OutputPin,"OutputPin"), /* Output/input relative to board. */ \
	_TC(Power,"Power"),\
	_TC(Stepper,"Stepper"),\
	_TC(Storage,"Storage"),\
	_TC(Thermistor,"Thermistor"),\
	/* Connection */ \
	_TC(Serial,"Serial"),\
	_TC(SPI,"SPI"),\
	_TC(I2C,"I2C"),\
	_TC(ADC,"ADC"),\
	_TC(PWM,"PWM"),\
	_TC(Misc,"Misc")

// Ugh, not ideal, but a dirty macro to generate references for string->enum and vice versa.
#define _TC(x,y) x
enum class TelCategory {
    TCENTRIES
};
#undef _TC
// A collection of "categories". A VCD object can
// list one or more of these when registering a trace.
// Only items in categries specified by a command line
// argument (or explicitly named) will be logged.



typedef const vector<TelCategory>& TelCats;
using TC = TelCategory;

class TelemetryHost: public BasePeripheral, public Scriptable
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

	void PrintTelemetry(bool bMarkdown = false);

	void SetCategories(const vector<string> &vsCats);

	// Convenience wrapper for scriptable BasePeripherals
	template<class C>
	inline void AddTrace(C* p, unsigned int eIRQ, TelCats vCats, uint8_t uiBits = 1)
	{
		AddTrace(p->GetIRQ(eIRQ),p->GetName(), vCats, uiBits);
	}

	LineStatus ProcessAction(unsigned int iAct, const vector<string> &vArgs) override;

	void AddTrace(avr_irq_t *pIRQ, string strName, TelCats vCats, uint8_t uiBits = 1);

	void Shutdown()
	{
		StopTrace();
	}

	private:
		TelemetryHost():Scriptable("TelHost")
		{
			if (m_pHost !=nullptr)
			{
				fprintf(stderr, "ERROR - duplicate initialization of telemetry host!\n");
				exit(1);
			}
			memset(&m_trace, 0, sizeof(m_trace));
			RegisterAction("WaitFor","Waits for a specified telemetry value to occur",ActWaitFor, {ArgType::String,ArgType::Int});
			RegisterActionAndMenu("StartTrace", "Starts the telemetry trace. You must have set a category or set of items with the -t option",ActStartTrace);
			RegisterActionAndMenu("StopTrace", "Stops a running telemetry trace.",ActStopTrace);
		}

		enum Actions
		{
			ActWaitFor,
			ActStartTrace,
			ActStopTrace
		};

		avr_vcd_t m_trace;

		vector<TelCategory> m_VLoglst;
		vector<string> m_vsNames;

		static TelemetryHost *m_pHost;

		map<string, avr_irq_t*>m_mIRQs;
		map<string, vector<TC>>m_mCatsByName;
		map<TC,vector<string>>m_mNamesByCat;

		avr_irq_t* m_pCurrentIRQ = nullptr;
		uint32_t m_uiMatchVal = 0;


		#define _TC(x,y) make_pair(y,TC::x)
		const map<string,TC> m_mStr2Cat = {
			TCENTRIES
		};
		#undef _TC
		#define _TC(x,y) make_pair(TC::x,y)
		const map<TC,string> m_mCat2Str = {
			TCENTRIES
		};
		#undef _TC

};
