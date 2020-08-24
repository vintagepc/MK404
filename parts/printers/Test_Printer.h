/*
	Test_Printer.h - Printer definition for the test printer.
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

#include "Test_Board.h"     		// for EinsyRambo
#include "Printer.h"        // for Printer, Printer::VisualType
#include "sim_avr.h"        // for avr_t
#include "sim_avr_types.h"  // for avr_io_addr_t

#include <atomic>           // for atomic_int
#include <cstdint>         // for uint32_t, uint8_t
#include <memory>           // for unique_ptr
#include <string>           // for string
#include <utility>          // for pair

class Test_Printer : public Boards::Test_Board, public Printer
{

	public:
		Test_Printer():Test_Board(),Printer(){};

		~Test_Printer(){};

		std::pair<int,int> GetWindowSize() override {return {0,0}; };

	protected:
		void SetupHardware() override;

		void OnAVRCycle() override;

	private:

};
