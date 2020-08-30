/*
	MMU1.h - MMU v1 helper
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

class MMU1 : public BasePeripheral
{
	public:
		#define IRQPAIRS _IRQ(MUX0,"<mux0.in") _IRQ(MUX1,"<mux1.in") _IRQ(TOOL_OUT, "8>tool_out")
		#include "IRQHelper.h"

		// Creates a logger that sniffs for
		MMU1() = default;

		// Shuts down the logger/closes file.
		~MMU1() = default;

		// Registers with SimAVR.
		void Init(avr_t *avr);

		inline std::string GetName(){return std::string("MMU1");}

	private:

		void OnMuxIn(avr_irq_t *irq, uint32_t value);


};
