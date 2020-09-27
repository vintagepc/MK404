/*
	I2CPeripheral.h - Generalization helper for I2C-based peripherals.
    This header auto-wires the bus and deals with some of the copypasta.
    Currently only supports bit-banged mode as I haven't needed to deal
	with "real" i2c hardware yet.

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
#include "avr_twi.h"
#include "sim_avr.h"         // for avr_t
#include "sim_io.h"          // for avr_io_getirq
#include "sim_irq.h"         // for avr_irq_t, avr_irq_register_notify
#include <cstdint>            // for uint8_t, uint32_t, int32_t, uint16_t
#include <iostream>

class I2CPeripheral: public BasePeripheral
{
    protected:
		explicit I2CPeripheral(uint8_t uiAddress);
		~I2CPeripheral() = default;

        // Sets up the IRQs on "avr" for this class. Optional name override IRQNAMES.
        template<class C>
        void _Init(avr_t *avr, C *p, const char** IRQNAMES = nullptr) {
            BasePeripheral::_Init(avr,p, IRQNAMES);

			std::cerr << "WARNING: UNIMPLEMENTED FEATURE - HARDWARE I2C\n";
            RegisterNotify(C::TX_IN, MAKE_C_CALLBACK(I2CPeripheral,_OnI2CTx<C>), this);
            ConnectFrom(avr_io_getirq(avr,AVR_IOCTL_TWI_GETIRQ(0),TWI_IRQ_OUTPUT), C::TX_IN); //NOLINT - complaint in external macro
            ConnectTo(C::TX_REPLY,avr_io_getirq(avr,AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_INPUT)); //NOLINT - complaint in external macro
        }

		// For bitbanged I2C connections. They are routed through standard TX/REPLY irqs so
		// your derived device doesn't have to care if it is on "real" I2C
		template<class C>
        void _Init(avr_t *avr, avr_irq_t *irqSDA, avr_irq_t *irqSCL, C *p, const char** IRQNAMES = nullptr) {
			BasePeripheral::_Init(avr,p, IRQNAMES);
			m_pSCL = irqSCL;
			m_pSDA = irqSDA;
			avr_irq_register_notify(irqSCL, MAKE_C_CALLBACK(I2CPeripheral,_OnSCL),this);
			avr_irq_register_notify(irqSDA, MAKE_C_CALLBACK(I2CPeripheral,_OnSDA),this);
		}

		// Override these for read and write operations on your device's registers.
		virtual uint8_t GetRegVal(uint8_t /*uiAddr*/){return 0;}; // pragma: LCOV_EXCL_LINE
		// Return T if success, F if failure. F results in NACK.
		virtual bool SetRegVal(uint8_t /*uiAddr*/, uint32_t /*uiData*/){return false;}; // pragma: LCOV_EXCL_LINE


    private:
		using I2CMsg_t = union {
			void I2CMsg_t(uint32_t uiRaw = 0){raw = uiRaw;}; // Convenience constructor
			void I2CMsg_t(const unsigned int &uiMsg, const unsigned &uiAddr, const unsigned &uiData){ raw = uiMsg<<16u | uiAddr << 8u | uiData;}
			uint32_t raw :24;
			uint8_t bytes[3] {0};
			struct {
				uint8_t data;
				uint8_t writeRegAddr;
				uint8_t isAddrRead :1; // Write bit on address. IDK if this is used given msgBits.isWrite.
				uint8_t address :7;
			};
		};

		// I2C transaction handler.
		template<class C>
		void _OnI2CTx(avr_irq_t */*irq*/, I2CMsg_t /*msg*/)
		{
			// TODO(vintagepc) - I don't have any real "hardware" i2c items to simulate at the moment.
		}

		// Called on a read request of uiReg. You don't need to worry about tracking/incrementing the address on multi-reads.
		bool ProcessByte(const uint8_t &value);
		void _OnSCL(avr_irq_t *irq, uint32_t value);
		void _OnSDA(avr_irq_t */*irq*/, uint32_t value);

		const uint8_t m_uiDevAddr = 0;

		avr_irq_t *m_pSCL = nullptr, *m_pSDA = nullptr;
		uint8_t m_uiByte = 0;

		I2CMsg_t msgIn;

		enum class State
		{
			Idle,
			AddrIn,
			RegIn,
			DataIn
		};

		enum class SCLS
		{
			Idle,
			Reading,
			Writing,
			WaitForWrite,
			WaitForACK
		};

		SCLS m_SCLState = SCLS::Idle;
		State m_state = State::Idle;

		uint8_t m_uiBitCt = 0;
};
