/*
	Button.h - simple button for SimAVR

	Original Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

	Rewritten/converted to c++ in 2020 by VintagePC <https://github.com/vintagepc/>

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

#include "BasePeripheral.h"    // for BasePeripheral, MAKE_C_TIMER_CALLBACK
#include "IKeyClient.h"
#include "IScriptable.h"       // for IScriptable::LineStatus
#include "Scriptable.h"        // for Scriptable
#include "sim_avr.h"           // for avr_t
#include "sim_avr_types.h"     // for avr_cycle_count_t
#include "sim_cycle_timers.h"  // for avr_cycle_timer_t
#include <cstdint>            // for uint32_t
#include <string>              // for string
#include <vector>              // for vector

class Button:public BasePeripheral, public Scriptable, private IKeyClient
{
	public:
	#define IRQPAIRS _IRQ(BUTTON_OUT,">button.out")
	#include "IRQHelper.h"

	// Creates a new button with name strName
	explicit Button(const std::string &strName = "Button");
	Button(const std::string &strName, const Key& key, const std::string& strDesc);

	// Initializes the button on "avr"
	void Init(struct avr_t * avr);

	// Presses the button for a given duration (us, default 1000)
	void Press(uint32_t uiUSec = 2000);

	// Turns the button into a toggle switch instead.
	void SetIsToggle(bool bVal);

	protected:
		LineStatus ProcessAction(unsigned int iAction, const std::vector<std::string> &vArgs) override;

		void OnKeyPress(const Key& key) override;

	private:
		avr_cycle_count_t AutoRelease(avr_t *avr, avr_cycle_count_t uiWhen);

		avr_cycle_timer_t m_fcnRelease = MAKE_C_TIMER_CALLBACK(Button,AutoRelease);

		std::string m_strName;

		enum Actions
		{
			ActPress,
			ActRelease,
			ActPressAndRelease
		};

		bool m_bIsToggle = false;

};
