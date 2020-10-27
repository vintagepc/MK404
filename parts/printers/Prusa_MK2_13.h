/*
	Prusa_MK2_13.h - Printer definition for the Prusa MK2 (mR 1.3)
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

#include "MiniRambo.h"     // for EinsyRambo
#include "MK3SGL.h"
#include "Printer.h"        // for Printer, Printer::VisualType
#include "sim_avr.h"        // for avr_t
#include "sim_avr_types.h"  // for avr_io_addr_t
#include <atomic>           // for atomic_int
#include <cstdint>         // for uint32_t, uint8_t
#include <memory>           // for unique_ptr
#include <utility>          // for pair

class MK3SGL;

class Prusa_MK2_13 : public Boards::MiniRambo, public Printer
{

	public:
		Prusa_MK2_13();

		~Prusa_MK2_13() override = default;

		void Draw() override;

		void OnMousePress(int button, int action, int x, int y) override;
		void OnMouseMove(int x,int y) override;

		void OnVisualTypeSet(const std::string &type) override;

		std::pair<int,int> GetWindowSize() override;

	protected:
		void SetupHardware() override;

		virtual void SetupPINDA();

		void OnAVRCycle() override;

		virtual bool GetHasSheet() { return false; }

		std::unique_ptr<MK3SGL> m_pVis {nullptr};


	private:
		void FixSerial(avr_t * avr, avr_io_addr_t addr, uint8_t v);

		std::atomic_int m_key = {0}, m_mouseBtn = {0};

};
