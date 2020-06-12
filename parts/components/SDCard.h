/*
	SDCard.h - an SD card SPI simulator.

	Original  Copyright (C) Philip Withnall 2012 <philip@tecnocode.co.uk> as part of Brewing Logger

	Modified for use with MK3SIM in 2020 by leptun <https://github.com/leptun/>

	Rewritten to C++ in 2020 by VintagePC <https://github.com/vintagepc/>

 	This file is part of MK3SIM.

	MK3SIM is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK3SIM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK3SIM.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>

#include "SPIPeripheral.h"

class SDCard:public SPIPeripheral
{
	public:
	#define IRQPAIRS \
	        _IRQ(SPI_BYTE_IN,       "8<SD.byte_in") \
	        _IRQ(SPI_BYTE_OUT,  	"8>SD.byte_out") \
            _IRQ(SPI_CSEL,          "<SD.cs_in") \
			_IRQ(CARD_PRESENT,		">SD.card_present")
		#include "IRQHelper.h"

		SDCard(std::string strFile = "SDCard.bin"):m_strFile(strFile){};

		void Init(avr_t *avr);

		// Mounts the given image file on the virtual card.
		// If size=0, autodetect the image size.
		// If filename is empty, remount the last file.
		int Mount(std::string filename = "", off_t image_size = 0);

		// Detaches the currently mounted file.
		int Unmount();

		inline bool IsMounted(){return m_data == nullptr;}

	protected:
		virtual uint8_t OnSPIIn(struct avr_irq_t * irq, uint32_t value) override;

        virtual void OnCSELIn(struct avr_irq_t * irq, uint32_t value) override;


	private:

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
		static const uint8_t R1_ADDRESS_OUT_OF_RANGE = (1 << 6);
		static const uint8_t R1_ADDRESS_MISALIGN = (1 << 5);
		static const uint8_t R1_ERASE_SEQ_ERROR = (1 << 4);
		static const uint8_t R1_COM_CRC_ERROR = (1 << 3);
		static const uint8_t R1_ILLEGAL_COMMAND = (1 << 2);
		static const uint8_t R1_ERASE_RESET = (1 << 1);
		static const uint8_t R1_IN_IDLE_STATE = (1 << 0);

		static const int READ_BL_LEN = 9;
		static const int BLOCK_SIZE = (1<<READ_BL_LEN); // Bytes
		static inline bool IsBlockAligned(int iBlock){ return ((iBlock % BLOCK_SIZE) == 0);};

		static inline uint32_t UINT8_ARRAY_TO_UINT32(const uint8_t a[4]){return ((((uint32_t)a[0]) << 24) | (((uint32_t) a[1]) << 16) | (((uint32_t) a[2]) << 8) | ((uint32_t) a[3]));}

		inline void CRC_ADD(const uint8_t data) {m_CRC = m_crctab[(m_CRC >> 8 ^ data) & 0XFF] ^ (m_CRC << 8); }

		/* TODO: See diskio.c */
		enum class Command{
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

		struct {
			Command command;
			uint8_t params[4];
			uint8_t checksum;
		} m_command_header;
		struct {
			uint8_t data[5];
			uint8_t length; /* number of bytes of data which are valid */
		} m_command_response;
		uint8_t m_command_index;
		bool m_bSelected = false;

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
		uint8_t m_csd[16]; /* card-specific data (CSD) register */

		uint16_t m_CRC;

		/* Card data. */
		uint8_t *m_data = nullptr; /* mmap()ed data */
		off_t m_data_length = 0;
		int m_data_fd = -1;
};
