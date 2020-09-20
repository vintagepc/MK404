/*
	Prusa_MK3S.h - Printer definition for the Prusa MK3S
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

#include "EinsyRambo.h"     // for EinsyRambo
#include "IRSensor.h"
#include "MK3SGL.h"
#include "Printer.h"        // for Printer, Printer::VisualType
#include "sim_avr.h"        // for avr_t
#include "sim_avr_types.h"  // for avr_io_addr_t
#include <atomic>           // for atomic_int
#include <cstdint>
#include <memory>           // for unique_ptr
#include <string>           // for string
#include <utility>          // for pair

class Prusa_MK3S : public Boards::EinsyRambo, public Printer
{

	public:
		Prusa_MK3S();

		~Prusa_MK3S() override = default;

		void Draw() override;
		void OnMousePress(int button, int action, int x, int y) override;
		void OnMouseMove(int x,int y) override;
		void OnVisualTypeSet(const std::string &type) override;

		std::pair<int,int> GetWindowSize() override;

	protected:
		void SetupHardware() override;

		IRSensor IR{};

		virtual void SetupIR(); // Overridden by the MK3 to setup the PAT.

		void OnAVRCycle() override;

		virtual bool GetHasMMU() {return false;}

		std::unique_ptr<MK3SGL> m_pVis {nullptr};

	private:
		void FixSerial(avr_t * avr, avr_io_addr_t addr, uint8_t v);

		std::atomic_int m_mouseBtn = {0};

};
