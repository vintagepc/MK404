/*
	Prusa_MMU2.h - Printer definition for a standalone virtual MMU.
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

#include "GCodeSniffer.h"
#include "MMU2.h"
#include "Printer.h"
#include <gsl-lite.hpp>

class MK3SGL;

#include <utility>          // for pair

class Prusa_MMU2 : public Printer, public MMU2
{
	public:

		Prusa_MMU2():MMU2(false){};

		~Prusa_MMU2() = default;

		inline std::pair<int,int> GetWindowSize() override;

		void Draw() override;

	protected:

		std::unique_ptr<MK3SGL> m_pVis {nullptr};

};
