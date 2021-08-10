/*
	IPCBoard.h - Board definition for the IPC printer. Just lightweight draw hardware only.
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

#include "Board.h"
#include "GLIndicator.h"
#include "GLMotor.h"
#include "wiring/Test_Wiring.h"                   // for Einsy_1_1a
#include <cstdint>
#include <memory>
#include <vector>                            // for uint32_t

namespace Boards
{
	class IPCBoard: public Board
	{
		public:
			explicit IPCBoard(uint32_t uiFreq = 16000000)
				:Board(m_wiring,uiFreq){ SetBoardName("IPC_Board"); };

		protected:
			std::vector<std::unique_ptr<GLMotor>> m_vMotors {};
			std::vector<std::unique_ptr<GLIndicator>> m_vInds {};
		private:

			const Wirings::Test_Wiring m_wiring = Wirings::Test_Wiring();
	};
}; // namespace Boards
