/*
	TelemetryHost.h - Wrangler for VCD traces and scripted telemetry

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
#include "IKeyClient.h"
#include "IScriptable.h"     // for ArgType, ArgType::Int, ArgType::String
#include "Scriptable.h"      // for Scriptable
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t
#include "sim_vcd_file.h"    // for avr_vcd_init, avr_vcd_start, avr_vcd_stop
#include <cstdint>          // for uint32_t, uint8_t
#include <map>               // for map
#include <string>            // for string
#include <vector>            // for vector

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
	_TC(Misc,"Misc"),\
	_TC(Mux,"Mux")

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

//typedef const std::vector<TelCategory>& TelCats;
using TelCats = const std::vector<TelCategory>&;
using TC = TelCategory;

class TelemetryHost: public BasePeripheral, public Scriptable, private IKeyClient
{
	public:

		enum IRQ {
			COUNT
		};

		const char *_IRQNAMES[IRQ::COUNT] = {
		};

		static TelemetryHost& GetHost()
		{
			static TelemetryHost h;
			return h;
		}

		// Inits the VCD file at the specified rate (in us)
		void Init(avr_t *pAVR, const std::string &strVCDFile, uint32_t uiRateUs = 100);

		inline void StartTrace()
		{
			avr_vcd_start(&m_trace);
		}

		inline void StopTrace()
		{
			avr_vcd_stop(&m_trace);
		}

		void PrintTelemetry(bool bMarkdown = false);

		void SetCategories(const std::vector<std::string> &vsCats);

		// Convenience wrapper for scriptable BasePeripherals
		template<class C>
		inline void AddTrace(C* p, unsigned int eIRQ, TelCats vCats, uint8_t uiBits = 1)
		{
			AddTrace(p->GetIRQ(eIRQ),p->GetName(), vCats, uiBits);
		}

		LineStatus ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs) override;

		void AddTrace(avr_irq_t *pIRQ, std::string strName, TelCats vCats, uint8_t uiBits = 1);

		inline void Shutdown()
		{
			StopTrace();
		}

		void OnKeyPress(const Key& key) override;

	private:
		TelemetryHost();

		enum Actions
		{
			ActWaitFor,
			ActWaitForGT,
			ActWaitForLT,
			ActIsEqual,
			ActStartTrace,
			ActStopTrace
		};

		avr_vcd_t m_trace {};

		std::vector<TelCategory> m_VLoglst;
		std::vector<std::string> m_vsNames;

		std::map<std::string, avr_irq_t*>m_mIRQs;
		std::map<std::string, std::vector<TC>>m_mCatsByName;
		std::map<TC,std::vector<std::string>>m_mNamesByCat;

		avr_irq_t* m_pCurrentIRQ = nullptr;
		uint32_t m_uiMatchVal = 0;


		#define _TC(x,y) {y,TC::x}
		const std::map<std::string,TC> m_mStr2Cat = {
			TCENTRIES
		};
		#undef _TC
		#define _TC(x,y) {TC::x,y}
		const std::map<TC,std::string> m_mCat2Str = {
			TCENTRIES
		};
		#undef _TC

};
