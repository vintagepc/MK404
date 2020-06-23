/*
	SDCard.cpp - an SD card SPI simulator.

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
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "SDCard.h"

static uint8_t CRC7(const uint8_t data[], size_t count)
{
	const uint8_t poly = 0b10001001;
	uint8_t crc = 0;
	size_t i =0;
	int j = 0;
	for (i = 0; i < count; i++) {
		crc ^= data[i];
		for (j = 0; j < 8; j++) {
			crc = (crc & 0x80u) ? ((crc << 1) ^ (poly << 1)) : (crc << 1);
		}
	}
	return crc | 0x01;
}

Scriptable::LineStatus SDCard::ProcessAction(unsigned int iAct, const vector<string> &vArgs)
{
	switch (iAct)
	{
		case ActUnmount:
			Unmount();
			return LineStatus::Finished;
		case ActMountLast:
			Mount();
			return LineStatus::Finished;
		case ActMountFile:
			return Mount(vArgs.at(0)) ? LineStatus::Error : LineStatus::Finished; // 0 = success.

	};
	return LineStatus::Unhandled;
}

inline void SDCard::COMMAND_RESPONSE_R1(uint8_t status)
{
	m_command_response.data[0] = status & 0x7f;
	m_command_response.length = 1;
}

inline void SDCard::COMMAND_RESPONSE_R3(uint8_t status, uint32_t payload)
{
	m_command_response.data[0] = (status & 0x7f);
	m_command_response.data[1] = (payload >> 24);
	m_command_response.data[2] = (payload >> 16);
	m_command_response.data[3] = (payload >> 8);
	m_command_response.data[4] = payload;
	m_command_response.length= 5;
}

const uint16_t SDCard::m_crctab[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/* Debug macros. */
// #define SD_CARD_DEBUG
#ifdef SD_CARD_DEBUG
#define DEBUG(m, ...) fprintf (stderr, "%lu: sdcard: " m "\n", avr->cycle, __VA_ARGS__);
#else
#define DEBUG(m, ...) do{}while(0)
#endif

/* Process the command currently in the command_header and emit a response. */
SDCard::State SDCard::ProcessCommand()
{
	SDCard::State next_state = State::IDLE;

	DEBUG ("Processing command %u.", m_CmdIn.bits.cmd);

	switch (m_CmdIn.bits.cmd) {
		case Command::CMD0:
			/* GO_IDLE_STATE. Return that we are in the idle state if we have a disk mounted; return an error otherwise. */
			/* TODO: checksum isn't checked */
			if (m_data == NULL) {
				COMMAND_RESPONSE_R1 (0xff);
			} else {
				COMMAND_RESPONSE_R1 (R1_IN_IDLE_STATE);
			}

			break;
		case Command::CMD8:
			/* State that we support SDv2 by returning 0x01 as the first byte, followed by an (arbitrary) 4-byte trailer. This is an R7
			 * response. The last 12 bits signify that the card can operate at a Vdd range of 2.7--3.6V. */
			m_command_response.data[0] = R1_IN_IDLE_STATE;
			m_command_response.data[1] = 0x00;
			m_command_response.data[2] = 0x00;
			m_command_response.data[3] = 0x01;
			m_command_response.data[4] = m_CmdIn.bytes[1];
			m_command_response.length = 5;
			break;
		case Command::CMD9:
			/* SEND_CSD. */
			COMMAND_RESPONSE_R1 (0x00);

			next_state = State::DATA_READ_TOKEN;
			read_bytes_remaining = 16;
			read_ptr = m_csd;

			break;
		case Command::CMD12:
			m_command_response.data[0] = 0x01;
			m_command_response.data[1] = 0x00;
			m_command_response.length = 2;
			break;
		case Command::CMD13: {
			/* SEND_STATUS. TODO: Hackily implemented and without reference to any data sheets. */
			m_command_response.data[0] = 0x00;
			m_command_response.data[1] = 0x00;
			m_command_response.length = 2;
			break;
		}
		case Command::CMD16: {
			/* SET_BLOCKLEN. */
			uint32_t blocklen;

			blocklen = m_CmdIn.bits.address;
			assert (blocklen == 512);
			/* TODO: only 512B blocks are supported at the moment. */

			COMMAND_RESPONSE_R1 (0x00);

			break;
		}
		case Command::CMD17: {
			off_t addr;

			/* READ_SINGLE_BLOCK. Reads a block of the size selected by the SET_BLOCKLEN command.
			 * Initiate a 512B read (TODO: we ignore SET_BLOCKLEN) from the address provided in the command. */
			addr = AddressToDataIdx(m_CmdIn.bits.address);

			DEBUG ("Read single block (CMD17) from address %lu.", addr);

			if (!IsBlockAligned(addr)) {
				/* Address misaligned. */
				COMMAND_RESPONSE_R1 (R1_ADDRESS_MISALIGN);
			} else if (addr > m_data_length) {
				/* Address out of range. */
				COMMAND_RESPONSE_R1 (R1_ADDRESS_OUT_OF_RANGE);
			} else {
				/* Success. Proceed to read the data. */
				COMMAND_RESPONSE_R1 (0x00);

				next_state = State::DATA_READ_TOKEN;
				read_ptr = m_data + addr;
				read_bytes_remaining = BLOCK_SIZE;
			}

			break;
		}
		case Command::CMD24: {
			off_t addr;

			/* WRITE_BLOCK. Writes a block of the size selected by the SET_BLOCKLEN command.
			 * TODO: we ignore SET_BLOCKLEN. */
			addr = AddressToDataIdx(m_CmdIn.bits.address);

			DEBUG ("Write block (CMD24) from address %lu.", addr);

			if (!IsBlockAligned(addr)) {
				/* Address misaligned. */
				COMMAND_RESPONSE_R1 (R1_ADDRESS_MISALIGN);
			} else if (addr > m_data_length) {
				/* Address out of range. */
				COMMAND_RESPONSE_R1 (R1_ADDRESS_OUT_OF_RANGE);
			} else {
				/* Success. Proceed to write the data. */
				COMMAND_RESPONSE_R1 (0x00);

				next_state = State::DATA_WRITE_TOKEN;
				write_ptr = m_data + addr;
				write_bytes_remaining = BLOCK_SIZE;
			}

			break;
		}
		case Command::CMD41:
			/* Application-specific. TODO: No idea what this does. */
			COMMAND_RESPONSE_R1 (0x00);
			break;
		case Command::CMD55:
			/* APP_CMD. Indicates to the card that the next command is an application specific command. */
			COMMAND_RESPONSE_R1 (0x00);
			break;
		case Command::CMD58:
			/* READ_OCR. */
			COMMAND_RESPONSE_R3 (0x00, m_ocr);
			break;
		default:
			/* Illegal command. */
			COMMAND_RESPONSE_R1 (R1_ILLEGAL_COMMAND);
			fprintf (stderr, "%lu: sdcard: Unknown SD card command ‘%u’.\n", m_pAVR->cycle, m_CmdIn.bits.cmd);
			break;
	}

	return next_state;
}

/* Called when the nSS IRQ is fired. */
void SDCard::OnCSELIn (struct avr_irq_t *irq, uint32_t value)
{
	m_bSelected = value==0;
	DEBUG ("SD card selected: %u. In state: %d", m_bSelected, m_state);
	if (!m_bSelected)
		m_state = State::IDLE;
}

uint8_t SDCard::OnSPIIn(struct avr_irq_t *irq, uint32_t value)
{
	DEBUG ("Received byte %x (in state %u).", value, m_state);
	uint8_t uiReply = 0xFF;
	/* Handle the command. */
	switch (m_state) {
		case State::IDLE:
			if (value == 0xff) {
				SetSendReplyFlag(); // Sends 0xFF
				break;
			} else {
				m_state = State::COMMAND_REQUEST;
				m_CmdCount = 0;

				/* Fall through. */
			}

			/* Fall through. */
		case State::COMMAND_REQUEST:
			/* Receive a 6-byte command header. */
			m_CmdIn.all<<=8;
			m_CmdIn.all |= (uint8_t)value;

			if (++m_CmdCount > 5) {
				/* If we've finished receiving the packet, process it and move to the response state. */
				m_state = State::COMMAND_RESPONSE;
				m_CmdCount = 0;
			}

			/* Dummy response for each of the header bytes. */
			uiReply = 0x00;
			SetSendReplyFlag();

			break;
		case State::COMMAND_RESPONSE: {
			State next_state = State::IDLE;

			/* Process the command. We do this here so that we can move directly into the DATA_READ state afterwards if required. */
			if (m_CmdCount == 0) {
				next_state = ProcessCommand();
			}

			/* Output the response stored in m_command_response. */
			if (m_CmdCount < m_command_response.length) {
				/* Outputting response bytes. */
				uiReply = m_command_response.data[m_CmdCount];
			}  // Else sends 0xFF
			SetSendReplyFlag();

			if (++m_CmdCount >= m_command_response.length) {
				/* Have we finished transmitting the response? */
				m_state = next_state;
			}

			break;
		}
		case State::DATA_READ_TOKEN:
			/* Output the data token. */
			uiReply = 0xfe;
			SetSendReplyFlag();
			m_state = State::DATA_READ;
			m_CRC = 0;
			break;
		case State::DATA_READ:
			/* Pump out data to the microcontroller. */
			uiReply = *(read_ptr);
			CRC_ADD(*(read_ptr));
			read_ptr++;
			SetSendReplyFlag();
			if (--read_bytes_remaining == 0) {
				/* Have we finished? */
				m_state = State::DATA_READ_CRC;
				read_bytes_remaining = 2;
			}

			break;
		case State::DATA_READ_CRC:
			/* Output the trailing CRC for a read. */
			uiReply = m_CRC >> (8 * (--read_bytes_remaining));
			SetSendReplyFlag();

			if (read_bytes_remaining == 0) {
				/* Have we outputted both bytes of the CRC? */
				m_state = State::IDLE;
			}

			break;
		case State::DATA_WRITE_TOKEN:
			/* Receive the data token. */
			/* TODO: We don't check the token is valid. */
			if (value == 0xfe) {
				/* Valid write token. */
				m_state = State::DATA_WRITE;
			}
			/* The microcontroller is waiting for us to be ready. */
			SetSendReplyFlag(); // Sends 0xFF

			break;
		case State::DATA_WRITE:
			/* Receive data from the microcontroller. */
			*(write_ptr++) = value;
			SetSendReplyFlag(); // Sends 0xFF

			if (--write_bytes_remaining == 0) {
				/* Have we finished? */
				m_state = State::DATA_WRITE_CRC;
				write_bytes_remaining = 2;
			}

			break;
		case State::DATA_WRITE_CRC:
			/* Receive the trailing CRC for a write. */
			/* TODO: check the CRC. */
			uiReply = 0x05;
			// if (write_bytes_remaining > 0) {
			// 	/* Write out two bytes to receive both bytes of the CRC. */
			// 	_sd_card_miso_send_byte (self, 0x05);
			// } else {
			// 	/* Write out our CRC status. *//* TODO: no idea what this means */
			// 	_sd_card_miso_send_byte (self, 0x05);
			// }
			SetSendReplyFlag();

			if (write_bytes_remaining-- == 0) {
				/* Have we received both bytes of the CRC and transmitted our response? */
				m_state = State::IDLE;
				write_bytes_remaining = 0;
			}
			break;
		default:
			/* Shouldn't be reached. */
			assert (0);
	}
	return uiReply;
}

void SDCard::InitCSD()
{
	memset(&m_csd, 0, sizeof(m_csd));

	const uint16_t CCC = 0x5B5;
	const uint8_t SECTOR_SIZE = 0x7F;
	const uint8_t WP_GRP_SIZE = 0x00;
	const uint8_t WRITE_BL_LEN = 9;

	m_csd[0] = 0b01 << 6; //CSD_STRUCTURE
	m_csd[1] = 0x0E; //(TAAC)
	m_csd[2] = 0x00; //(NSAC)
	m_csd[3] = 0x32; //(TRAN_SPEED)
	m_csd[4] = CCC >> 4; //CCC MSB
	m_csd[5] = (CCC & 0xF) << 4; //CCC LSB
	m_csd[5] |= READ_BL_LEN & 0xF; //(READ_BL_LEN)
	m_csd[10] = (1 << 6); //(ERASE_BLK_EN)
	m_csd[10] |= SECTOR_SIZE >> 1; // (SECTOR_SIZE MSB)
	m_csd[11] = (uint8_t)(SECTOR_SIZE << 7); // (SECTOR_SIZE LSB)
	m_csd[11] |= WP_GRP_SIZE; //(WP_GRP_SIZE)
	m_csd[12] |= 0 << 7; //(WP_GRP_ENABLE)
	m_csd[12] = 0x02 << 2; //(R2W_FACTOR)
	m_csd[12] |= WRITE_BL_LEN >> 2; //(WRITE_BL_LEN MSB)
	m_csd[13] = (uint8_t)(WRITE_BL_LEN << 6); //(WRITE_BL_LEN LSB)
	m_csd[13] |= 0 << 5; //(WRITE_BL_PARTIAL)
	m_csd[14] = 0 << 7; //(FILE_FORMAT_GRP)
	m_csd[14] |= 1 << 6; //COPY
	m_csd[14] |= 0 << 5; //PERM_WRITE_PROTECT
	m_csd[14] |= 0 << 4; //TMP_WRITE_PROTECT
	m_csd[14] |= 0 << 2; //(FILE_FORMAT)
	m_csd[15] = CRC7(m_csd, sizeof(m_csd) - 1);
}

void SDCard::SetCSDCSize(off_t c_size)
{
	assert((c_size % (512 * 1024)) == 0);

	const uint32_t C_SIZE = c_size / (512 * 1024);
	assert ((C_SIZE >> 16) == 0); //limit of 32GB

	m_csd[9] |= (C_SIZE);
	m_csd[8] |= (C_SIZE >> 8);
	m_csd[7] |= (C_SIZE >> 16);
	m_csd[15] = CRC7(m_csd, sizeof(m_csd) - 1);
#ifdef SD_CARD_DEBUG
	printf("CSD: ");
	for (int i = 0; i < sizeof(m_csd); i++)
	{
		printf("%02hX", m_csd[i]);
	}
	printf("\n");
#endif //SD_CARD_DEBUG
}

void SDCard::Init(struct avr_t *avr)
{
	_Init(avr,this);

	m_ocr |= 1 << 30; //SDHC
	m_ocr |= 1 << 31; //Card power up status bit

	InitCSD();
}

int SDCard::Mount(const std::string &filename, off_t image_size)
{
	int fd = 0;
	void *mapped;
	bool bLocked = false; /* boolean */

	auto OnError  = [fd, bLocked](int err)
	{
		/* Clean up after an error. */
		if (bLocked) {
			flock (fd, LOCK_UN);
		}
		close (fd);
		return err;
	};

	struct stat stat_buf;
	if (!filename.empty())
		m_strFile = filename; // New file given.

	/* Open the specified disk image. */
	fd = open (m_strFile.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);

	if (fd == -1)
		return errno;

	/* Lock it for exclusive access. */
	if (flock (fd, LOCK_EX) == -1)
		return OnError(errno);

	bLocked = true;

	/* Check its size. If it's smaller than the requested size, expand it. Otherwise, ignore any excess size. */
	if (fstat (fd, &stat_buf) == -1)
		return OnError(errno);

	if (image_size == 0)
	{
		image_size = stat_buf.st_size;
		if (image_size==0)
		{
			printf("No SD image found. Aborting mount.\n");
			return OnError(-1);
		}
		printf("Autodetected SD image size as %lu Mb\n",image_size>>20); // >>20 = div by 1024*1024
	}
	else if (stat_buf.st_size < image_size)
	{
		if (ftruncate (fd, image_size) == -1)
			return OnError(errno);
	}

	/* Map it into memory. */
	mapped = mmap (NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (mapped == MAP_FAILED)
		return OnError(errno);

	/* Success. */
	m_data = (uint8_t*)mapped;
	m_data_length = image_size;
	m_data_fd = fd;

	/* Update the C_SIZE field (number of sectors) in the CSD register. Reference for size calculations: JESD84-A44, Section 8.3, 'C_SIZE'. */
	SetCSDCSize(image_size);

	m_bMounted = true;
	RaiseIRQ(CARD_PRESENT,0);

	return 0;
}

int SDCard::Unmount()
{
	if (m_data == nullptr) {
		/* No disk mounted. */
		m_bMounted = false;
		return 0;
	}

	/* Synchronise changes. */
	msync (m_data, m_data_length, MS_SYNC | MS_INVALIDATE);

	/* Unlock the file. */
	flock (m_data_fd, LOCK_UN);

	/* munmap() and close. */
	munmap (m_data, m_data_length);
	close (m_data_fd);

	m_data = nullptr;
	m_data_length = 0;
	m_data_fd = -1;

	m_bMounted = false;
	InitCSD();
	SetCSDCSize(0);
	RaiseIRQ(CARD_PRESENT,1);
	return 0;
}
