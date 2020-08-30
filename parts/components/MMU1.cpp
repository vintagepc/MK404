/*
	MMU1.cpp - Handler for MMU v1 on a MK2.
	Its main purpose is for sniffing T-codes from the MMU. But it might have use elsewhere...

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

#include "MMU1.h"
#include "TelemetryHost.h"
//#include <iostream>     // for printf

void MMU1::Init(struct avr_t * avr)
{
	_Init(avr, this);

	RegisterNotify(MUX0, MAKE_C_CALLBACK(MMU1, OnMuxIn),this);
	RegisterNotify(MUX1, MAKE_C_CALLBACK(MMU1, OnMuxIn),this);


	TelemetryHost::GetHost().AddTrace(this, TOOL_OUT, {TC::Misc},8);
}

void MMU1::OnMuxIn(avr_irq_t *irq, uint32_t value)
{
	uint8_t uiTool = 0;
	if (irq == GetIRQ(MUX0))
	{
		uiTool = (2*(GetIRQ(MUX1)->value))+value;
	}
	else
	{
		uiTool = (2*(value))+GetIRQ(MUX0)->value;
	}
	RaiseIRQ(TOOL_OUT,uiTool);
}
