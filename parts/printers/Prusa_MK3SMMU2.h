/*
	Prusa_MK3SMMU2.h - Printer definition for the Prusa MK3S w/MMU2
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

#include "Prusa_MK3S.h"
#include "SerialPipe.h"
#include "MMU2.h"

class Prusa_MK3SMMU2 : public Prusa_MK3S
{

	public:
		template<typename ...Args>
		Prusa_MK3SMMU2(Args... args):Prusa_MK3S(args...){};
		~Prusa_MK3SMMU2();

		void Draw() override;
		void OnVisualTypeSet(VisualType type) override;

		bool GetHasMMU() override {return true;}

		std::pair<int,int> GetWindowSize() override;

	protected:
		void SetupHardware() override;

		void OnMMUFeed(avr_irq_t *irq, uint32_t value);// Helper for MMU IR sensor triggering.

		MMU2 m_MMU;
		SerialPipe *m_pipe = nullptr;

	private:


};
