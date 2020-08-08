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

#include <stdint.h>         // for uint32_t, uint8_t
#include <atomic>
#include <memory>           // for unique_ptr
#include "EinsyRambo.h"     // for EinsyRambo
#include "Printer.h"        // for Printer, Printer::VisualType
#include "sim_avr.h"        // for avr_t
#include "sim_avr_types.h"  // for avr_io_addr_t
#include "IRSensor.h"
#include <utility>
#include "MK3SGL.h"

class Prusa_MK3S : public Boards::EinsyRambo, public Printer
{

	public:
		Prusa_MK3S():EinsyRambo(),Printer(){};

		~Prusa_MK3S(){};

		void Draw() override;
		virtual void OnKeyPress(unsigned char key, int x, int y) override;
		void OnMousePress(int button, int action, int x, int y) override;
		void OnMouseMove(int x,int y) override;
		void OnVisualTypeSet(string type) override;

		std::pair<int,int> GetWindowSize() override;

	protected:
		void SetupHardware() override;

		IRSensor IR{};

		virtual void SetupIR(); // Overridden by the MK3 to setup the PAT.
		inline virtual void ToggleFSensor(){ IR.Toggle(); };

		inline virtual void FSensorJam() {};

		void OnAVRCycle() override;

		virtual bool GetHasMMU() {return false;}

		std::unique_ptr<MK3SGL> m_pVis;

	private:
		void FixSerial(avr_t * avr, avr_io_addr_t addr, uint8_t v);

		std::atomic_int m_key = {0}, m_mouseBtn = {0};

		unsigned int m_iScheme = 0;
		uint32_t m_colors[8] = {
		0x02c5fbff, 0x8d7ff8ff, 0xFFFFFFff, 0x00000055,
		0x382200ff, 0x000000ff , 0xFF9900ff, 0x00000055};


};
