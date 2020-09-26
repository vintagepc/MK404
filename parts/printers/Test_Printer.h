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

#include "HD44780GL.h"
#include "Printer.h"        // for Printer, Printer::VisualType
#include "Test_Board.h"     		// for EinsyRambo
#include <utility>          // for pair

class Test_Printer : public Boards::Test_Board, public Printer
{

	public:
		Test_Printer():Test_Board(),Printer(){};

		~Test_Printer() override = default;

		std::pair<int,int> GetWindowSize() override {return { 5 + m_lcd.GetWidth() * 6, 50 + 5 + m_lcd.GetHeight() * 9}; };

		void Draw() override;

	protected:
		void SetupHardware() override;

		void OnAVRCycle() override;

};
