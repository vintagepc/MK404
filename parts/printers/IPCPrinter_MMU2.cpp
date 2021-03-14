/*
	IPCPrinter_MMU2.cpp - IPC+MMU2... IDK, maybe this is useful for someone.
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

#include "IPCPrinter_MMU2.h"
#include "MMU2.h"

void IPCPrinter_MMU2::SetupHardware()
{
	IPCPrinter::SetupHardware();
	m_MMU.StartAVR();
}

void IPCPrinter_MMU2::Draw()
{
	glPushMatrix();
		IPCPrinter::Draw();
		m_MMU.Draw(static_cast<float>(GetWindowSize().second));
	glPopMatrix();
}
