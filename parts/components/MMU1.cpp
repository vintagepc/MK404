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
#include "Util.h"
#include <algorithm>        // for copy

MMU1::MMU1():GLIndicator('0', false, true) {
	SetColor(GetToolColor(0));
	SetValue(255);
}

void MMU1::Init(struct avr_t * avr)
{
	_Init(avr, this);

	RegisterNotify(MUX0, MAKE_C_CALLBACK(MMU1, OnMuxIn),this);
	RegisterNotify(MUX1, MAKE_C_CALLBACK(MMU1, OnMuxIn),this);
	RegisterNotify(STEP_IN, MAKE_C_CALLBACK(MMU1, OnStepIn),this);
	for (auto i=0u; i<4; i++)
	{
		GetIRQ(STEP0+i)->flags |= IRQ_FLAG_FILTERED;
	}

	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, TOOL_OUT, {TC::Misc},8);
	TH.AddTrace(this, STEP0, {TC::Stepper});
	TH.AddTrace(this, STEP1, {TC::Stepper});
	TH.AddTrace(this, STEP2, {TC::Stepper});
	TH.AddTrace(this, STEP3, {TC::Stepper});
	SetVisible(true);
}

void MMU1::OnStepIn(avr_irq_t */*irq*/, uint32_t value)
{
	RaiseIRQ(STEP0+m_uiTool, value);
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
	m_uiTool = uiTool;
	SetColor(GetToolColor(uiTool));
	SetLabel('0'+uiTool);
	//printf("TOOL: %u\n", uiTool);
	RaiseIRQ(TOOL_OUT,uiTool);
}

hexColor_t MMU1::GetToolColor(uint8_t uiTool)
{
	switch (uiTool)
	{
		case 0:
			return 0xFF0000FF;
		case 1:
			return 0x00FF00FF;
		case 2:
			return 0x0000FFFF;
		default:
			return 0xFFFFFFFF;
	}
}
