/*
	HC595.h - a shift register for the MMU.

	Original SPI portion Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

	Rewritten for C++ and a bitbanged connection in 2020, VintagePC <https://github.com/vintagepc/>

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

#include "BasePeripheral.h"  // for BasePeripheral
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t
#include <cstdint>          // for uint32_t, uint8_t
#include <string>

/*
 * this one is quite fun, it simulated a 74HC595 shift register
 * driven by an SPI signal.
 * For the interest of the simulation, they can be chained, but
 * for practicality sake the shift register is kept 32 bits
 * wide so it acts as 4 of them "daisy chained" already.
 */

class HC595: public BasePeripheral
{
	public:

		#define IRQPAIRS \
			_IRQ(SPI_BYTE_IN,"8<hc595.spi_in") \
			_IRQ(SPI_BYTE_OUT,"8>hc595.spi_out") \
			_IRQ(IN_LATCH,"32<hc595.latch_in") \
			_IRQ(IN_RESET,"<hc595.reset_in") \
			_IRQ(IN_CLOCK,"<hc595.clock_in") \
			_IRQ(IN_DATA,"<hc595.data_in") \
			_IRQ(SHIFT_OUT,"32>hc595.out") \
			_IRQ(BIT0	,">bit0") \
			_IRQ(BIT1	,">bit1") \
			_IRQ(BIT2	,">bit2") \
			_IRQ(BIT3	,">bit3") \
			_IRQ(BIT4	,">bit4") \
			_IRQ(BIT5	,">bit5") \
			_IRQ(BIT6	,">bit6") \
			_IRQ(BIT7	,">bit7") \
			_IRQ(BIT8	,">bit8") \
			_IRQ(BIT9	,">bit9") \
			_IRQ(BIT10	,">bit10") \
			_IRQ(BIT11	,">bit11") \
			_IRQ(BIT12	,">bit12") \
			_IRQ(BIT13	,">bit13") \
			_IRQ(BIT14	,">bit14") \
			_IRQ(BIT15	,">bit15") \
			_IRQ(BIT16	,">bit16") \
			_IRQ(BIT17	,">bit17") \
			_IRQ(BIT18	,">bit18") \
			_IRQ(BIT19	,">bit19") \
			_IRQ(BIT20	,">bit20") \
			_IRQ(BIT21	,">bit21") \
			_IRQ(BIT22	,">bit22") \
			_IRQ(BIT23	,">bit23") \
			_IRQ(BIT24	,">bit24") \
			_IRQ(BIT25	,">bit25") \
			_IRQ(BIT26	,">bit26") \
			_IRQ(BIT27	,">bit27") \
			_IRQ(BIT28	,">bit28") \
			_IRQ(BIT29	,">bit29") \
			_IRQ(BIT30	,">bit30") \
			_IRQ(BIT31	,">bit31")
		#include "IRQHelper.h"

		// Registers with SimAVR
		void Init(avr_t *avr);

		inline const std::string GetName(){return "HC595";}

	private:
		// IRQ handlers.
		void OnLatchIn(avr_irq_t *irq, uint32_t value);
		void OnClockIn(avr_irq_t *irq, uint32_t value);
		void OnSPIIn(avr_irq_t *irq, uint32_t value);
		void OnDataIn(avr_irq_t *irq, uint32_t value);
		void OnResetIn(avr_irq_t *irq, uint32_t value);

		uint32_t	m_uiLatch = 0;		// value "on the pins"
		uint32_t 	m_uiValue = 0;		// value shifted in
		uint8_t		m_uiCurBit =0;
		//uint8_t		m_uiCurByte = 0;

};
