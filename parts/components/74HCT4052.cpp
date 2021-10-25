/*
	74HCT4052.cpp - Mux component sim.

	Copyright 2021 VintagePC <https://github.com/vintagepc/>

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

#include "74HCT4052.h"
#include "BasePeripheral.h"
#include "TelemetryHost.h"
#include <iostream>

uint32_t L74HCT4052::OnADCRead(struct avr_irq_t* /*irq*/, uint32_t)
{
	uint8_t sel = ((GetIRQ(B_IN)->value)<<1U) | GetIRQ(A_IN)->value;
	// give it a chance to respond - attached to SRC 255 so it doesn't bind a REAL adc pin.
	union {
		avr_adc_mux_t t;
		uint32_t raw;
	} mux = {{0}};
	mux.t.src = 0xFFU;
	RaiseIRQ(OUT_0 + sel,mux.raw);
	uint8_t uiIndex = IN_0 + sel;
	// Return value.
	// printf("Reading %u from chan %u\n",GetIRQ(uiIndex)->value, sel);
    return GetIRQ(uiIndex)->value;
}

void L74HCT4052::Init(struct avr_t * avr, uint8_t adc_mux_number)
{
	_Init(avr, adc_mux_number, this);

	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, ADC_VALUE_OUT, {TC::ADC, TC::Mux},16);
	TH.AddTrace(this, A_IN,{TC::Mux});
	TH.AddTrace(this, B_IN,{TC::Mux});
	TH.AddTrace(this, OUT_0,{TC::Mux});
	TH.AddTrace(this, OUT_1,{TC::Mux});
	TH.AddTrace(this, OUT_2,{TC::Mux});
	TH.AddTrace(this, OUT_3,{TC::Mux});
}

