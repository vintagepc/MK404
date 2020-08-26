/*
	Prusa_MK3SMMU2.h - Printer definition for the Prusa MK3S w/MMU2
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

#include "GCodeSniffer.h"  // for GCodeSniffer
#include "IRSensor.h"      // for IRSensor, IRSensor::IRState::IR_AUTO
#include "MMU2.h"          // for MMU2
#include "Prusa_MK3S.h"    // for Prusa_MK3S
#include "SerialPipe.h"
#include "sim_irq.h"       // for avr_irq_t
#include <cstdint>        // for uint32_t
#include <memory>
#include <string>          // for string
#include <utility>         // for pair

class Prusa_MK3SMMU2 : public Prusa_MK3S
{

	public:
		Prusa_MK3SMMU2():Prusa_MK3S(){};
		~Prusa_MK3SMMU2();

		void Draw() override;
		void OnVisualTypeSet(std::string type) override;

		bool GetHasMMU() override {return true;}

		std::pair<int,int> GetWindowSize() override;

		void OnKeyPress(unsigned char key, int x, int y) override;

	protected:
		void SetupHardware() override;

		inline virtual void FSensorResumeAuto() { IR.Set(IRSensor::IR_AUTO);}

		void OnMMUFeed(avr_irq_t *irq, uint32_t value);// Helper for MMU IR sensor triggering.

		MMU2 m_MMU;
		GCodeSniffer m_sniffer = GCodeSniffer('T');
		unique_ptr<SerialPipe> m_pipe {nullptr};

	private:


};
