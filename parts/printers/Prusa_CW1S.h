/*
	Prusa_CW1S.h - Printer definition for the CW1S
	Copyright 2021 VintagePC <https://github.com/vintagepc/>
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

#include "boards/CW1S.h"
#include "Printer.h"
#include <atomic>
#include <gsl-lite.hpp>

class Prusa_CW1S : public Printer, public Boards::CW1S
{
	public:

		virtual std::pair<int,int> GetWindowSize() override;

		void Draw() override;

		void OnMousePress(int button, int action, int x, int y) override;

	protected:
		void OnAVRCycle() override;

	private:
		std::atomic_int m_mouseBtn = {0};



};
