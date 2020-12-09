/*
	IPCPrinter.h - Printer definition for the IPC dummy printer.
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

#include "Printer.h"        // for Printer, Printer::VisualType
#include "IPCBoard.h"     		// for EinsyRambo
#include <gsl-lite.hpp>
#include <fstream>
#include <utility>          // for pair

class IPCPrinter : public Boards::IPCBoard, public Printer
{

	public:
		IPCPrinter():IPCBoard(),Printer(){};

		~IPCPrinter() override;

		inline std::pair<int,int> GetWindowSize() override
		{
			auto height = 10*(m_vMotors.size()+1);
			return {125, height};
		}

		void Draw() override;

	protected:
		void SetupHardware() override;

		void OnAVRCycle() override;

		void AddPart(const std::string& strIn);

		void UpdateMotor();
		void UpdateIndicator();

		std::ifstream m_ifIn;

		char _m_msg[256] {0};
		gsl::span<char> m_msg {_m_msg};
		uint16_t m_len = 0;

};
