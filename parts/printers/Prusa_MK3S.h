/*
	Prusa_MK3S.h - Printer definition for the Prusa MK3S
	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK3SIM.

	MK3SIM is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK3SIM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK3SIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "EinsyRambo.h"
#include "Printer.h"
#include "MK3SGL.h"

class Prusa_MK3S : public Boards::EinsyRambo, public Printer
{

	public:
		Prusa_MK3S():EinsyRambo(){};
		Prusa_MK3S(std::string strFW):EinsyRambo(strFW){};
		Prusa_MK3S(bool bConnSerial):EinsyRambo(),Printer(bConnSerial){};
		Prusa_MK3S(std::string strFW,bool bConnSerial):EinsyRambo(strFW),Printer(bConnSerial){};

		void Draw() override;
		void OnKeyPress(unsigned char key, int x, int y) override;
		void OnMousePress(int button, int action, int x, int y) override;
		void OnMouseMove(int x,int y) override;
		void OnVisualTypeSet(VisualType type) override;

		std::pair<int,int> GetWindowSize() override;

	protected:
		void SetupHardware() override;

		void OnAVRCycle() override;

		virtual bool GetHasMMU() {return false;}

		MK3SGL *m_pVis = nullptr;

	private:
		void FixSerial(avr_t * avr, avr_io_addr_t addr, uint8_t v);

		unsigned char m_key = 0, m_mouseBtn = 0;

		unsigned int m_iScheme = 0;
		uint32_t m_colors[8] = {
		0x02c5fbff, 0x8d7ff8ff, 0xFFFFFFff, 0x00000055,
		0x382200ff, 0x000000ff , 0xFF9900ff, 0x00000055};


};
