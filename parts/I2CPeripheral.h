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

#include <cstdint>            // for uint8_t, uint32_t, int32_t, uint16_t
#include "BasePeripheral.h"
#include <avr_twi.h>

class I2CPeripheral: public BasePeripheral
{
    protected:
		I2CPeripheral(uint8_t uiAddress):m_uiDevAddr(uiAddress){};
		~I2CPeripheral(){};

        // Sets up the IRQs on "avr" for this class. Optional name override IRQNAMES.
        template<class C>
        void _Init(avr_t *avr, C *p, const char** IRQNAMES = nullptr) {
            BasePeripheral::_Init(avr,p, IRQNAMES);

			fprintf(stderr,"WARNING: UNIMPLEMENTED FEATURE - HARDWARE I2C\n");
            RegisterNotify(C::TX_IN, MAKE_C_CALLBACK(I2CPeripheral,_OnI2CTx<C>), this);
            ConnectFrom(avr_io_getirq(avr,AVR_IOCTL_TWI_GETIRQ(0),TWI_IRQ_OUTPUT), C::TX_IN);
            ConnectTo(C::TX_REPLY,avr_io_getirq(avr,AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_INPUT));
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
		virtual uint8_t GetRegVal(uint8_t uiAddr){return 0;};
		// Return T if success, F if failure. F results in NACK.
		virtual bool SetRegVal(uint8_t uiAddr, uint32_t uiData){return false;};


    private:
		typedef union I2CMsg_t {
			I2CMsg_t(uint32_t uiRaw = 0){ raw = uiRaw; }; // Convenience constructor
			I2CMsg_t(const uint8_t &uiMsg, const uint8_t &uiAddr, const uint8_t &uiData){ raw = uiMsg<<16 | uiAddr << 8 | uiData;}
			uint32_t raw :24;
			uint8_t bytes[3];
			struct {
				uint8_t data;
				uint8_t writeRegAddr;
				uint8_t isAddrRead :1; // Write bit on address. IDK if this is used given msgBits.isWrite.
				uint8_t address :7;
			};
		} I2CMsg_t;

		// I2C transaction handler.
		template<class C>
		void _OnI2CTx(avr_irq_t *irq, I2CMsg_t msg)
		{
			// TODO - I don't have any real "hardware" i2c items to simulate at the moment.
		}

		// Called on a read request of uiReg. You don't need to worry about tracking/incrementing the address on multi-reads.
		//virtual uint8_t ProcessRead(uint8_t uiReg){};
		bool ProcessByte(const uint8_t &value)
		{
			bool bReturn = false;
			switch (m_state)
			{
				case State::AddrIn:
				{
					msgIn.bytes[2] = value;
					if (msgIn.address != m_uiDevAddr)
					{
						m_SCLState = SCLS::Idle;
						m_state = State::Idle;
						break;
					}
					bReturn = true;
					if (msgIn.isAddrRead)
					{
						m_SCLState = SCLS::WaitForWrite; // Wait for next clock
					}
					else
					{
						m_state = State::RegIn;
					}
				}
				break;
				case State::RegIn:
				{
					msgIn.bytes[1] = value;
					m_state = State::DataIn;
					bReturn = true;
					// TODO- this doesn't handle OOB regs.
				}
				break;
				case State::DataIn:
				{
					msgIn.bytes[0] = value;
					bReturn = SetRegVal(msgIn.writeRegAddr++,msgIn.data);
				}
				break;
				default:
				break;
			}
			//printf("I2C msg status: Addr: %02x Read: %u raw: %04x\n", msgIn.address, msgIn.isAddrRead, msgIn.raw);
			return bReturn;
		}

		void _OnSCL(avr_irq_t *irq, uint32_t value)
		{
			//if (value) printf("SCL: %u (%u)\n", value,m_uiBitCt);
			bool bClockRise = !(irq->value) && value;
			bool bClockFall = (irq->value) && !value;
			if  (m_SCLState ==SCLS::Idle || (bClockRise==bClockFall))
				return;
			switch (m_SCLState)
			{
				default:
					break;
				case SCLS::Reading:
					if (bClockRise)
					{
						if (m_uiBitCt == 8) // Byte done, do we ACK or NACK?
						{
							if (ProcessByte(m_uiByte))
							{
								//printf("ACK\n");
								avr_raise_irq(m_pSDA,0);
							}
							// else
							// 	printf("NACK\n");
							m_uiByte = m_uiBitCt = 0;
						}
						else
						{
							m_uiByte <<= 1; // Shift up.
							m_uiByte |= (m_pSDA->value & 1);
							m_uiBitCt++;
						}
					}
					break;
				case SCLS::WaitForWrite:
					if (bClockFall)
						m_SCLState = SCLS::Writing;
					break;
				case SCLS::Writing:
					{
						if (bClockRise)
						{
							if (m_uiBitCt == 0)
							{
								m_uiBitCt = 8;
								m_uiByte = GetRegVal(msgIn.writeRegAddr);
								//printf("Sending %02x\n",m_uiByte);
							}
							avr_raise_irq(m_pSDA, m_uiByte>>(--m_uiBitCt) &1);
						}
						else
						{
							if (m_uiBitCt == 0)
								m_SCLState = SCLS::WaitForACK;
						}
					}
					break;
				case SCLS::WaitForACK:
					{
						if (bClockRise)
						{
							//printf("%cACK rec'd\n",m_pSDA->value? ' ':'N');
						}
						else
						{
							m_SCLState = SCLS::Writing; // Back to writing.
						}
					}
					break;
			}
		}

		void _OnSDA(avr_irq_t *irq, uint32_t value)
		{
			if (m_pSCL->value)
			{
				if (value)
					m_SCLState = SCLS::Idle;
				else
				{
					m_SCLState = SCLS::Reading;
					m_state = State::AddrIn;
					m_uiBitCt = 0;
					m_uiByte = 0;
				}
				//printf("I2C TX %s\n",value?"Stop" : "Start");
			}
		}

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
