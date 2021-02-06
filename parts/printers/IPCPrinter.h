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

#include "BasePeripheral.h"
#include "Printer.h"        // for Printer, Printer::VisualType
#include "IPCBoard.h"     		// for EinsyRambo
#include "MK3SGL.h"
#include <gsl-lite.hpp>
#include <fstream>
#ifdef MQ
	#include <mqueue.h>
#else
extern "C" {
	#include "../../3rdParty/shmemq-blog/shmemq.h"
}
#endif
#include <utility>          // for pair

class IPCPrinter : public Boards::IPCBoard, public Printer, public BasePeripheral
{
	public:

		#define IRQPAIRS _IRQ(PINDA_OUT,">pinda.out") _IRQ(BED_OUT,">bed.out") \
			 _IRQ(X_STEP_OUT,"x_step_out") _IRQ(X_POSITION_OUT,">x.out") \
			 _IRQ(Y_STEP_OUT,"y_step_out") _IRQ(Y_POSITION_OUT,">y.out") \
			 _IRQ(Z_STEP_OUT,"z_step_out") _IRQ(Z_POSITION_OUT,">z.out") \
			 _IRQ(E_STEP_OUT,"e_step_out") _IRQ(E_POSITION_OUT,">e.out")
		#include "IRQHelper.h"


		IPCPrinter():IPCBoard(),Printer(){};

		~IPCPrinter() override;

		inline std::pair<int,int> GetWindowSize() override
		{
			auto height = 10*(m_vMotors.size()+1);
			return {125, height};
		}
		void OnVisualTypeSet(const std::string &type) override;

		void Draw() override;

	protected:
		void SetupHardware() override;

		void OnAVRCycle() override;

		void AddPart(const std::string& strIn);

		void UpdateMotor();
		void UpdateIndicator();

		std::unique_ptr<MK3SGL> m_pVis {nullptr};

		std::ifstream m_ifIn;

		char _m_msg[256] {0};
		gsl::span<char> m_msg {_m_msg};
		uint16_t m_len = 0;

		std::vector<unsigned int> m_vStepIRQs, m_vIndIRQs;

		std::array<uint32_t,4> m_vStepsPerMM = {100,100,400,280};
#ifdef MQ
		mqd_t m_queue;
		struct mq_attr m_qAttr;
#endif
		shmemq_t *m_queue = nullptr;
};
