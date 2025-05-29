/*
	Prusa_MK25SMMU2.h - Printer definition for the Prusa MK3S w/MMU2
	Copyright 2025 VintagePC <https://github.com/vintagepc/>

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

#include "GCodeSniffer.h"  // for GCodeSniffer
#include "MMU2.h"          // for MMU2
#include "Prusa_MK25S_13.h"    // for Prusa_MK25S_13
#include "SerialPipe.h"
#include "sim_irq.h"       // for avr_irq_t
#include <cstdint>        // for uint32_t
#include <memory>
#include <string>          // for string
#include <utility>         // for pair
#include "Prusa_MMUTypes.h"  // for MMUType

class Prusa_MK25SMMU2 : public Prusa_MK25S_13
{

	public:
		Prusa_MK25SMMU2():Prusa_MK25S_13(){};
		~Prusa_MK25SMMU2() override = default;

		void Draw() override;
		void OnVisualTypeSet(const std::string &type) override;

		inline MMUType GetHasMMU() override {return MMUType::MMUv2;}

		std::pair<int,int> GetWindowSize() override;

	protected:
		void SetupHardware() override;

		void OnMMUFeed(avr_irq_t *irq, uint32_t value);// Helper for MMU IR sensor triggering.

		MMU2 m_MMU;
		GCodeSniffer m_sniffer = GCodeSniffer('T');
		std::unique_ptr<SerialPipe> m_pipe {nullptr};

	private:


};
