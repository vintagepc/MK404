/*
	Wiring.h - wiring definition that maps from common names to specific pins for your board.
	Derive from this and set the values accordingly for your model, -1 is undefined/unused.

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

#include "PinNames.h"
#include "PinSpec.h"
#include "avr_ioport.h"
#include "avr_timer.h"
#include <map>


namespace Wirings
{
	using namespace PinNames; //NOLINT - because proper using declarations don't support enums.
	class Wiring
	{
		public:
			// Making a type so that this is easy to update
			// down the road if we need more values than this can provide.
			using MCUPin = signed char;
			using Pin = PinNames::Pin;

			// Creates a new board with the given pinspec.
			explicit Wiring(const PinSpec &pSpec):m_pinSpec(pSpec){};

			// Passthrough to retrieve the MCU name
			std::string GetMCUName() const { return m_pinSpec.GetMCUName(); }

			const PinSpec& GetPinSpec() const { return m_pinSpec; }

			virtual ~Wiring() = default;

			// Shorthand to get IRQ for a digital IO port.
			inline struct avr_irq_t* IOIRQ(struct avr_t* avr,uint8_t port,uint8_t number) const { return avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ(port),number); }

			// Timer PWM shorthand function
			inline struct avr_irq_t* TIMERIRQ(struct avr_t* avr,uint8_t timer,uint8_t number) const {return avr_io_getirq(avr,AVR_IOCTL_TIMER_GETIRQ(timer), _TimerPWMID(number)); }

			// Looks up a digital IRQ based on the arduino convenience pin number.
			//inline struct avr_irq_t* DIRQLU(struct avr_t* avr, unsigned int n){ return IOIRQ(avr,m_pinSpec.PORT(n),m_pinSpec.PIN(n)); }
			inline struct avr_irq_t* DIRQLU(struct avr_t* avr, Pin ePin) const { return IOIRQ(avr,m_pinSpec.PORT(m_mPins.at(ePin)),m_pinSpec.PIN(m_mPins.at(ePin))); }

			// Looks up a PWM IRQ based on the arduino convenience pin number.
			//inline struct avr_irq_t* DPWMLU(struct avr_t* avr, unsigned int n) { return TIMERIRQ(avr, m_pinSpec.TIMER_CHAR(n), m_pinSpec.TIMER_IDX(n)); }
			inline struct avr_irq_t* DPWMLU(struct avr_t* avr, Pin ePin)  const { return TIMERIRQ(avr, m_pinSpec.TIMER_CHAR(m_mPins.at(ePin)), m_pinSpec.TIMER_IDX(m_mPins.at(ePin))); }

			// Returns T/F whether a pin is defined.
			inline bool IsPin(PinNames::Pin pin) const { return m_mPins.count(pin)>0;}

			inline MCUPin GetPin(PinNames::Pin ePin) const { return m_mPins.at(ePin);}

		protected:
			// Gets the PWM id for a given timer number.
			inline unsigned int _TimerPWMID(uint8_t number) const { return TIMER_IRQ_OUT_PWM0 + number; }

			std::map<Pin,MCUPin> m_mPins;

		private:
			const PinSpec &m_pinSpec;
	};
}; // namespace Wirings
