/*
	SPIPeripheral.h - Generalization helper for SPI-based peripherals.
    This header auto-wires the SPI and deals with some of the copypasta
    relating to checking CSEL and so on. You just need to have
    OnSPIIn and (optionally) OnCSELIn overriden, as well as the
    SPI_BYTE_[*]/SPI_CSEL IRQs defined.

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
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t
#include <cstdint>          // for uint32_t, uint8_t

class SPIPeripheral: public BasePeripheral
{
	friend BasePeripheral;
    protected:

        // SPI input helper. Overload this in your SPI class.
        // If you want to send a reply, return the value and call SetSendReplyFlag()
        virtual uint8_t OnSPIIn(struct avr_irq_t * irq, uint32_t value) = 0;

        // SPI CSEL helper. You can overload this if you want, but you don't need to for
        // basic 8-bit SPI objects as it already guards OnSPIIn.
        virtual void OnCSELIn(struct avr_irq_t * irq, uint32_t value) = 0;

        // Sets the flag that you have and want to send a reply.
        inline void SetSendReplyFlag(){m_bSendReply = true;}

        // Sets up the IRQs on "avr" for this class
		void OnPostInit(avr_t* avr, unsigned int eCSEL);

    private:
        bool m_bCSel = true; // Chipselect, active low.
        bool m_bSendReply = false;

        void _OnCSELIn(struct avr_irq_t * irq, uint32_t value);

        void _OnSPIIn(struct avr_irq_t * irq, uint32_t value);

		avr_irq_t* m_pSPIReply = nullptr;
};
