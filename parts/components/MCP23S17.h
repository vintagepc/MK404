/*
	MCP23S17.h - simulated GPIO extender.

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


#pragma once

#include "BasePeripheral.h"    // for MAKE_C_TIMER_CALLBACK
#include "SPIPeripheral.h"     // for SPIPeripheral
#include "sim_irq.h"           // for avr_irq_t
#include <cstdint>            // for uint8_t, uint32_t, int32_t, uint16_t
#include <string>              // for string
#include <vector>              // for vector

typedef struct {
	uint8_t IODIR;		//0x00
	uint8_t IPOL;		//0x01
	uint8_t GPINTEN;	//0x02
	uint8_t DEFVAL;		//0x03
	uint8_t INTCON;		//0x04
	union {
		uint8_t iocon;		//0x05
		struct {
			uint8_t :1;
			uint8_t INTPOL :1;
			uint8_t ODR :1;
			uint8_t HAEN :1;
			uint8_t DISSLW :1;
			uint8_t SEQOP :1;
			uint8_t MIRROR :1;
			uint8_t BANK :1;
		} __attribute__ ((__packed__)) IOCON;
	};
	uint8_t GPPU;		//0x06
	uint8_t INTF;		//0x07
	uint8_t INTCAP;		//0x08
	uint8_t GPIO;		//0x09
	uint8_t OLAT;		//0x0A
	uint8_t _unused[5];	//0x0B-F
} mcp_reg_bank_1_t;

class MCP23S17: public SPIPeripheral
{
    public:
        #define IRQPAIRS \
	        _IRQ(SPI_BYTE_IN,       "8<mcp23s17.byte_in") \
	        _IRQ(SPI_BYTE_OUT,  	"8>mcp23s17.byte_out") \
            _IRQ(SPI_CSEL,          "<mcp23s17.cs_in")\
			_IRQ(MCP_GPA0, "<>mcp23s17.a0") \
			_IRQ(MCP_GPA1, "<>mcp23s17.a1") \
			_IRQ(MCP_GPA2, "<>mcp23s17.a2") \
			_IRQ(MCP_GPA3, "<>mcp23s17.a3") \
			_IRQ(MCP_GPA4, "<>mcp23s17.a4") \
			_IRQ(MCP_GPA5, "<>mcp23s17.a5") \
			_IRQ(MCP_GPA6, "<>mcp23s17.a6") \
			_IRQ(MCP_GPA7, "<>mcp23s17.a7") \
			_IRQ(MCP_GPB0, "<>mcp23s17.b0") \
			_IRQ(MCP_GPB1, "<>mcp23s17.b1") \
			_IRQ(MCP_GPB2, "<>mcp23s17.b2") \
			_IRQ(MCP_GPB3, "<>mcp23s17.b3") \
			_IRQ(MCP_GPB4, "<>mcp23s17.b4") \
			_IRQ(MCP_GPB5, "<>mcp23s17.b5") \
			_IRQ(MCP_GPB6, "<>mcp23s17.b6") \
			_IRQ(MCP_GPB7, "<>mcp23s17.b7")
        #include "IRQHelper.h"

        // Default constructor.
        explicit MCP23S17();
		virtual ~MCP23S17() = default;

        // Registers with SimAVR.
        void Init(avr_t *avr);

		inline const std::string GetName(){return "MCP23S17";}

    private:
		// Lookup for bank mode to internal reg order
		uint8_t ToBank1Addr(uint8_t addr);

		// handles a write. Addr MUST be bank1-style.
		void OnWrite(uint8_t b1_addr, uint8_t value);

		void OnODRChanged(uint8_t bank, uint8_t old, uint8_t value);

		void OnPinChanged(struct avr_irq_t * irq,uint32_t value);

        // SPI handlers.
        uint8_t OnSPIIn(avr_irq_t *irq, uint32_t value) override;
        void OnCSELIn(avr_irq_t *irq, uint32_t value) override;
		enum class MCP_STATE{
			IDLE,
			OPCODE,
			ADDR,
			DATA,
		};
		MCP_STATE m_state = MCP_STATE::IDLE;

		uint8_t m_addr;
		union {
			uint8_t opcode;
			struct {
				uint8_t READ :1;
				uint8_t HADDR :3;
				uint8_t _FIXED :4;
			} __attribute__ ((__packed__));
		} m_hdr;

		union {
			uint8_t raw[0x20];
			mcp_reg_bank_1_t bank[2];
		} m_regs;

		bool m_bSelf = false;

};
