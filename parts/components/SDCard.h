/*
	SDCard.h - an SD card SPI simulator.

	Original  Copyright (C) Philip Withnall 2012 <philip@tecnocode.co.uk> as part of Brewing Logger

	Modified for use with MK404 in 2020 by leptun <https://github.com/leptun/>

	Rewritten to C++ in 2020 by VintagePC <https://github.com/vintagepc/>

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

#include "IScriptable.h"    // for ArgType, ArgType::String, IScriptable::Li...
#include "SPIPeripheral.h"  // for SPIPeripheral
#include "Scriptable.h"     // for Scriptable
#include "gsl-lite.hpp"
#include "sim_avr.h"        // for avr_t
#include <cstdint>         // for uint8_t, uint32_t, uint16_t, uint64_t
#include <string>           // for string
#include <sys/types.h>      // for off_t
#include <vector>           // for vector

class SDCard:public SPIPeripheral, public Scriptable
{
	public:
	#define IRQPAIRS \
	        _IRQ(SPI_BYTE_IN,       "8<SD.byte_in") \
	        _IRQ(SPI_BYTE_OUT,  	"8>SD.byte_out") \
            _IRQ(SPI_CSEL,          "<SD.cs_in") \
			_IRQ(CARD_PRESENT,		">SD.card_present")
		#include "IRQHelper.h"

		explicit SDCard(std::string strFile = "SDCard.bin");

		void Init(avr_t *avr);

		inline void SetImage(const string &strFile) { m_strFile = strFile;}

		// Mounts the given image file on the virtual card.
		// If size=0, autodetect the image size.
		// If filename is empty, remount the last file.
		int Mount(const std::string &filename = "", off_t image_size = 0);

		// Detaches the currently mounted file.
		int Unmount();

		inline bool IsMounted(){return m_bMounted; }

	protected:
		uint8_t OnSPIIn(struct avr_irq_t * irq, uint32_t value) override;

        void OnCSELIn(struct avr_irq_t * irq, uint32_t value) override;

		LineStatus ProcessAction(unsigned int iAct, const vector<string> &vArgs) override;


	private:

		enum Actions
		{
			ActMountFile,
			ActMountLast,
			ActUnmount
		};

		enum class State {
			IDLE,
			COMMAND_REQUEST,
			COMMAND_RESPONSE,
			DATA_READ_TOKEN,
			DATA_READ,
			DATA_READ_CRC,
			DATA_WRITE_TOKEN,
			DATA_WRITE,
			DATA_WRITE_CRC,
		};

		SDCard::State ProcessCommand();

		void InitCSD();
		void SetCSDCSize(off_t size);

		inline void COMMAND_RESPONSE_R1(uint8_t uiR1Status);
		inline void COMMAND_RESPONSE_R3(uint8_t uiR3Status, uint32_t payload);

		/* Convert from an address as received in a command header, to a byte offset into the data array. */
		static inline off_t AddressToDataIdx (off_t input_address){ return input_address * BLOCK_SIZE;}

		/* Bit fields for R1 responses. Reference: JESD84-A44, Section 7.13. */
		static const uint8_t R1_ADDRESS_OUT_OF_RANGE = 0x40; // (1u << 6);
		static const uint8_t R1_ADDRESS_MISALIGN = 0x20; // (1u << 5);
		static const uint8_t R1_ERASE_SEQ_ERROR = 0x10; //(1u << 4);
		static const uint8_t R1_COM_CRC_ERROR = 0x08; //(1u << 3);
		static const uint8_t R1_ILLEGAL_COMMAND = 0x04; // (1u << 2);
		static const uint8_t R1_ERASE_RESET = 0x02; //(1u << 1);
		static const uint8_t R1_IN_IDLE_STATE = 0x01; //(1u << 0);

		static const uint8_t READ_BL_LEN = 9;
		static const unsigned int BLOCK_SIZE = (1U<<READ_BL_LEN); // Bytes
		static inline bool IsBlockAligned(int iBlock){ return ((iBlock % BLOCK_SIZE) == 0);};

		inline void CRC_ADD(const uint8_t data) {m_CRC = m_crctab[(m_CRC >> 8u ^ data) & 0XFFu] ^ (m_CRC << 8u); }

		/* TODO: See diskio.c */
		enum Command {
			CMD0 = 0,
			CMD8 = 8,
			CMD9 = 9,
			CMD12 = 12,
			CMD13 = 13,
			CMD16 = 16,
			CMD17 = 17,
			CMD24 = 24,
			CMD41 = 41,
			CMD55 = 55,
			CMD58 = 58,
		};

		static const uint16_t m_crctab[];

		std::string m_strFile;

		/* Internal state. */
		State m_state = State::IDLE;
		union {
			uint64_t all : 48;
			struct {
				uint8_t checksum :8;
				unsigned int address :32;
				Command cmd :6;
				unsigned char :2; // Start/position
			} __attribute__ ((__packed__)) bits;
			uint8_t bytes[6];
		} m_CmdIn {.all = 0};

		uint8_t m_CmdCount = 0;

		struct {
			uint8_t data[5];
			uint8_t length; /* number of bytes of data which are valid */
		} m_command_response {{0}, 0};

		bool m_bSelected = false, m_bMounted = false;

		struct m_currOp
		{
			inline void SetData(const gsl::span<uint8_t> &in){data = in; pos = in.begin();};
			inline bool IsFinsihed(){ return pos==data.end(); }
			gsl::span<uint8_t> data;
			gsl::span<uint8_t>::iterator pos {nullptr};
		}m_currOp;

		union {
			/* Ongoing read operations. */
			struct {
				uint8_t *read_ptr;
				uint32_t read_bytes_remaining;
			};

			/* Ongoing write operations. */
			struct {
				uint8_t *write_ptr;
				uint32_t write_bytes_remaining;
			};
		};

		/* Internal registers. */
		uint32_t m_ocr = 0; /* operation conditions register (OCR) */
		uint8_t _m_csd[16] = {0}; /* card-specific data (CSD) register */
		gsl::span<uint8_t> m_csd;

		uint16_t m_CRC = 0;
		uint8_t _m_ByteCRC[2] = {0,0};
		gsl::span<uint8_t> m_byteCRC {_m_ByteCRC};

		/* Card data. */
		gsl::span<uint8_t> m_data; /* mmap()ed data */
		int m_data_fd = -1;
};
