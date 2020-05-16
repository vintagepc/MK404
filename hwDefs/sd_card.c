/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Brewing Logger.
 * Copyright (C) Philip Withnall 2012 <philip@tecnocode.co.uk>
 *
 * Brewing Logger is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Brewing Logger is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Brewing Logger.  If not, see <http://www.gnu.org/licenses/>.
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

/* Various #defines needed for simavr. */
#define AVR_STACK_WATCH 0
#define __AVR__ 0

#include <avr_spi.h>
#include <sim_avr.h>

// #include "../config.h"
#include "sd_card.h"

/* TODO: See diskio.c */
typedef enum {
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
} SdCardCommand;

static const uint16_t crctab[] = {
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

static void CRC_ADD(const uint8_t data, void *param) {
	sd_card_t *self = (sd_card_t *) param;
	self->CRC = crctab[(self->CRC >> 8 ^ data) & 0XFF] ^ (self->CRC << 8);
}

#define COMMAND_RESPONSE_R1(status) \
	self->command_response.data[0] = ((status) & 0x7f); \
	self->command_response.length = 1;
#define COMMAND_RESPONSE_R3(status, payload) \
	self->command_response.data[0] = ((status) & 0x7f); \
	self->command_response.data[1] = ((payload) >> 24); \
	self->command_response.data[2] = ((payload) >> 16); \
	self->command_response.data[3] = ((payload) >> 8); \
	self->command_response.data[4] = (payload); \
	self->command_response.length= 5;

/* Bit fields for R1 responses. Reference: JESD84-A44, Section 7.13. */
#define R1_ADDRESS_OUT_OF_RANGE (1 << 6)
#define R1_ADDRESS_MISALIGN (1 << 5)
#define R1_ERASE_SEQ_ERROR (1 << 4)
#define R1_COM_CRC_ERROR (1 << 3)
#define R1_ILLEGAL_COMMAND (1 << 2)
#define R1_ERASE_RESET (1 << 1)
#define R1_IN_IDLE_STATE (1 << 0)

#define READ_BL_LEN 9 /* log2(max. read block length); see JESD84-A44, Section 8.3, 'READ_BL_LEN'. */
#define BLOCK_SIZE (1 << READ_BL_LEN) /* bytes */
#define IS_BLOCK_ALIGNED(a) ((a) % BLOCK_SIZE == 0)

#define UINT8_ARRAY_AS_UINT32(a) ((((uint32_t) (a)[0]) << 24) | (((uint32_t) (a)[1]) << 16) | (((uint32_t) (a)[2]) << 8) | ((uint32_t) (a)[3]))

/* Debug macros. */
// #define SD_CARD_DEBUG
#ifdef SD_CARD_DEBUG
#define DEBUG(m, ...) fprintf (stderr, "%lu: sdcard: " m "\n", avr->cycle, __VA_ARGS__);
#else
#define DEBUG(m, ...) do{}while(0)
#endif

// #define SDv1
// #define SDv2
#define SDHC //high capacity

/* Convert from an address as received in a command header, to a byte offset into the data array. */
static off_t _sd_card_input_address_to_data_index (off_t input_address)
{
#ifdef SDHC
	return input_address * BLOCK_SIZE;
#else
	return input_address;
#endif
}

/* Process the command currently in the command_header and emit a response. */
static sd_card_state_t _sd_card_process_command (struct avr_t *avr, sd_card_t *self)
{
	sd_card_state_t next_state = SD_CARD_IDLE;

	DEBUG ("Processing command %u.", self->command_header.command);

	switch (self->command_header.command) {
		case CMD0:
			/* GO_IDLE_STATE. Return that we are in the idle state if we have a disk mounted; return an error otherwise. */
			/* TODO: checksum isn't checked */
			if (self->data == NULL) {
				COMMAND_RESPONSE_R1 (0xff);
			} else {
				COMMAND_RESPONSE_R1 (R1_IN_IDLE_STATE);
			}

			break;
#ifndef SDv1
		case CMD8:
			/* State that we support SDv2 by returning 0x01 as the first byte, followed by an (arbitrary) 4-byte trailer. This is an R7
			 * response. The last 12 bits signify that the card can operate at a Vdd range of 2.7--3.6V. */
			self->command_response.data[0] = R1_IN_IDLE_STATE;
			self->command_response.data[1] = 0x00;
			self->command_response.data[2] = 0x00;
			self->command_response.data[3] = 0x01;
			self->command_response.data[4] = self->command_header.params[3];
			self->command_response.length = 5;
			break;
#endif
		case CMD9:
			/* SEND_CSD. */
			COMMAND_RESPONSE_R1 (0x00);

			next_state = SD_CARD_DATA_READ_TOKEN;
			self->read_bytes_remaining = 16;
			self->read_ptr = self->csd;

			break;
		case CMD12:
			self->command_response.data[0] = 0x01;
			self->command_response.data[1] = 0x00;
			self->command_response.length = 2;
			break;
		case CMD13: {
			/* SEND_STATUS. TODO: Hackily implemented and without reference to any data sheets. */
			self->command_response.data[0] = 0x00;
			self->command_response.data[1] = 0x00;
			self->command_response.length = 2;
			break;
		}
		case CMD16: {
			/* SET_BLOCKLEN. */
			uint32_t blocklen;

			blocklen = UINT8_ARRAY_AS_UINT32 (self->command_header.params);
			assert (blocklen == 512);
			/* TODO: only 512B blocks are supported at the moment. */

			COMMAND_RESPONSE_R1 (0x00);

			break;
		}
		case CMD17: {
			off_t addr;

			/* READ_SINGLE_BLOCK. Reads a block of the size selected by the SET_BLOCKLEN command.
			 * Initiate a 512B read (TODO: we ignore SET_BLOCKLEN) from the address provided in the command. */
			addr = _sd_card_input_address_to_data_index (UINT8_ARRAY_AS_UINT32 (self->command_header.params));

			DEBUG ("Read single block (CMD17) from address %lu.", addr);

			if (!IS_BLOCK_ALIGNED (addr)) {
				/* Address misaligned. */
				COMMAND_RESPONSE_R1 (R1_ADDRESS_MISALIGN);
			} else if (addr > self->data_length) {
				/* Address out of range. */
				COMMAND_RESPONSE_R1 (R1_ADDRESS_OUT_OF_RANGE);
			} else {
				/* Success. Proceed to read the data. */
				COMMAND_RESPONSE_R1 (0x00);

				next_state = SD_CARD_DATA_READ_TOKEN;
				self->read_ptr = self->data + addr;
				self->read_bytes_remaining = BLOCK_SIZE;
			}

			break;
		}
		case CMD24: {
			off_t addr;

			/* WRITE_BLOCK. Writes a block of the size selected by the SET_BLOCKLEN command.
			 * TODO: we ignore SET_BLOCKLEN. */
			addr = _sd_card_input_address_to_data_index (UINT8_ARRAY_AS_UINT32 (self->command_header.params));

			DEBUG ("Write block (CMD24) from address %lu.", addr);

			if (!IS_BLOCK_ALIGNED (addr)) {
				/* Address misaligned. */
				COMMAND_RESPONSE_R1 (R1_ADDRESS_MISALIGN);
			} else if (addr > self->data_length) {
				/* Address out of range. */
				COMMAND_RESPONSE_R1 (R1_ADDRESS_OUT_OF_RANGE);
			} else {
				/* Success. Proceed to write the data. */
				COMMAND_RESPONSE_R1 (0x00);

				next_state = SD_CARD_DATA_WRITE_TOKEN;
				self->write_ptr = self->data + addr;
				self->write_bytes_remaining = BLOCK_SIZE;
			}

			break;
		}
		case CMD41:
			/* Application-specific. TODO: No idea what this does. */
			COMMAND_RESPONSE_R1 (0x00);
			break;
		case CMD55:
			/* APP_CMD. Indicates to the card that the next command is an application specific command. */
			COMMAND_RESPONSE_R1 (0x00);
			break;
		case CMD58:
			/* READ_OCR. */
			COMMAND_RESPONSE_R3 (0x00, self->ocr);
			break;
		default:
			/* Illegal command. */
			COMMAND_RESPONSE_R1 (R1_ILLEGAL_COMMAND);
			fprintf (stderr, "%lu: sdcard: Unknown SD card command ‘%u’.\n", avr->cycle, self->command_header.command);
			break;
	}

	return next_state;
}

/* Send a byte on the MISO line. */
static void _sd_card_miso_send_byte (sd_card_t *self, uint8_t value)
{
	struct avr_t *avr = self->avr;

	avr_raise_irq (self->irq + IRQ_SD_CARD_MISO, value);
	DEBUG ("Transmitted byte %x (in state %u).", value, self->state);
}

/* Called when the nSS IRQ is fired. */
static void _sd_card_ss_pin_changed_cb (struct avr_irq_t *irq, uint32_t value, void *param)
{
	sd_card_t *self = (sd_card_t *) param;
	struct avr_t *avr = self->avr;

	self->selected = (value == 0) ? 1 : 0; /* invert the value, since nSS is active low */
	DEBUG ("SD card selected: %u. In state: %d", self->selected, self->state);
	if (self->selected == 0)
		self->state = SD_CARD_IDLE;
}

/* Called when the MOSI IRQ is fired. */
static void _sd_card_mosi_pin_changed_cb (struct avr_irq_t *irq, uint32_t value, void *param)
{
	sd_card_t *self = (sd_card_t *) param;
	struct avr_t *avr = self->avr;

	/* Bail if this chip isn't selected. */
	if (self->selected == 0) {
		return;
	}

	DEBUG ("Received byte %x (in state %u).", value, self->state);

	/* Handle the command. */
	switch (self->state) {
		case SD_CARD_IDLE:
			if (value == 0xff) {
				_sd_card_miso_send_byte (self, 0xff);
				break;
			} else {
				self->state = SD_CARD_COMMAND_REQUEST;
				self->command_index = 0;

				/* Fall through. */
			}

			/* Fall through. */
		case SD_CARD_COMMAND_REQUEST:
			/* Receive a 6-byte command header. */
			if (self->command_index == 0) {
				self->command_header.command = (value & 0x3f);
			} else if (self->command_index == 5) {
				self->command_header.checksum = value;
			} else {
				self->command_header.params[self->command_index - 1] = value;
			}

			if (++self->command_index > 5) {
				/* If we've finished receiving the packet, process it and move to the response state. */
				self->state = SD_CARD_COMMAND_RESPONSE;
				self->command_index = 0;
			}

			/* Dummy response for each of the header bytes. */
			_sd_card_miso_send_byte (self, 0x00);

			break;
		case SD_CARD_COMMAND_RESPONSE: {
			sd_card_state_t next_state = SD_CARD_IDLE;

			/* Process the command. We do this here so that we can move directly into the DATA_READ state afterwards if required. */
			if (self->command_index == 0) {
				next_state = _sd_card_process_command (self->avr, self);
			}

			/* Output the response stored in self->command_response. */
			if (self->command_index < self->command_response.length) {
				/* Outputting response bytes. */
				_sd_card_miso_send_byte (self, self->command_response.data[self->command_index]);
			} else {
				/* Outputting error bytes. */
				_sd_card_miso_send_byte (self, 0xff);
			}

			if (++self->command_index >= self->command_response.length) {
				/* Have we finished transmitting the response? */
				self->state = next_state;
			}

			break;
		}
		case SD_CARD_DATA_READ_TOKEN:
			/* Output the data token. */
			_sd_card_miso_send_byte (self, 0xfe);
			self->state = SD_CARD_DATA_READ;
			self->CRC = 0;
			break;
		case SD_CARD_DATA_READ:
			/* Pump out data to the microcontroller. */
			_sd_card_miso_send_byte (self, *(self->read_ptr));
			CRC_ADD(*(self->read_ptr), param);
			self->read_ptr++;

			if (--self->read_bytes_remaining == 0) {
				/* Have we finished? */
				self->state = SD_CARD_DATA_READ_CRC;
				self->read_bytes_remaining = 2;
			}

			break;
		case SD_CARD_DATA_READ_CRC:
			/* Output the trailing CRC for a read. */
			_sd_card_miso_send_byte (self, self->CRC >> (8 * (--self->read_bytes_remaining)));

			if (self->read_bytes_remaining == 0) {
				/* Have we outputted both bytes of the CRC? */
				self->state = SD_CARD_IDLE;
			}

			break;
		case SD_CARD_DATA_WRITE_TOKEN:
			/* Receive the data token. */
			/* TODO: We don't check the token is valid. */
			if (value == 0xfe) {
				/* Valid write token. */
				_sd_card_miso_send_byte (self, 0xff);
				self->state = SD_CARD_DATA_WRITE;
			}

			/* The microcontroller is waiting for us to be ready. */
			_sd_card_miso_send_byte (self, 0xff);

			break;
		case SD_CARD_DATA_WRITE:
			/* Receive data from the microcontroller. */
			*(self->write_ptr++) = value;
			_sd_card_miso_send_byte (self, 0xff);

			if (--self->write_bytes_remaining == 0) {
				/* Have we finished? */
				self->state = SD_CARD_DATA_WRITE_CRC;
				self->write_bytes_remaining = 2;
			}

			break;
		case SD_CARD_DATA_WRITE_CRC:
			/* Receive the trailing CRC for a write. */
			/* TODO: check the CRC. */
			if (self->write_bytes_remaining > 0) {
				/* Write out two bytes to receive both bytes of the CRC. */
				_sd_card_miso_send_byte (self, 0x05);
			} else {
				/* Write out our CRC status. *//* TODO: no idea what this means */
				_sd_card_miso_send_byte (self, 0x05);
			}

			if (self->write_bytes_remaining-- == 0) {
				/* Have we received both bytes of the CRC and transmitted our response? */
				self->state = SD_CARD_IDLE;
				self->write_bytes_remaining = 0;
			}

			break;
		default:
			/* Shouldn't be reached. */
			assert (0);
	}
}

static const char *_irq_names[IRQ_SD_CARD_COUNT] = {
	[IRQ_SD_CARD_MOSI] = "8<sdcard.mosi",
	[IRQ_SD_CARD_MISO] = "8>sdcard.miso",
	[IRQ_SD_CARD_nSS] = "<sdcard.nss",
	[IRQ_SD_CARD_PRESENT] = ">sdcard.present"
};

/* Set the C_SIZE field of the CSD register. Reference: JESD84-A44, Section 8.3. */
static void _sd_card_set_csd_c_size (sd_card_t *self, uint16_t c_size /* only LS 10b used */, uint8_t c_size_mult /* only LS 3b used */)
{
	/* Check they fit within the LS 12b and LS 3b. */
	assert ((c_size & 0xfff) == c_size);
	assert ((c_size_mult & 0x07) == c_size_mult);

	/* Set C_SIZE. */
	self->csd[6] = (self->csd[6] & 0xfc) | ((c_size >> 10) & 0x03); /* LS 2b form MS 2b of C_SIZE field. */
	self->csd[7] = (c_size >> 2) & 0xff; /* Middle 8b of C_SIZE field. */
	self->csd[8] = ((c_size & 0x03) << 6) | (self->csd[8] & 0x3f); /* MS 2b form LS 2b of C_SIZE field. */

	/* Set C_SIZE_MULT. */
	self->csd[9] = (self->csd[9] & 0xfc) | ((c_size_mult >> 1) & 0x03); /* LS 2b form MS 2b of C_SIZE_MULT field. */
	self->csd[10] = ((c_size_mult & 0x01) << 7) | (self->csd[10] & 0x7f); /* MS 1b forms LS 1b of C_SIZE_MULT field. */
}

void sd_card_init (struct avr_t *avr, sd_card_t *p)
{
	memset (p, 0, sizeof (*p));

	p->avr = avr;
	p->state = SD_CARD_IDLE;

	p->ocr= 0;
#ifdef SDHC
	p->ocr |= 1 << 30; //SDHC
#endif
	p->ocr |= 1 << 31; //Card power up status bit

	/* CSD register. Reference: JESD84-A44, Section 8.3. TODO: Haven't look at all the fields yet. */
	/* Most significant byte. */
	p->csd[0] = 0x00; /* CSD version 1.1 */
	p->csd[5] = (READ_BL_LEN & 0x0f);
	_sd_card_set_csd_c_size (p, 0, 0);
	/* Least significant byte. */

	p->data = NULL; /* no disk mounted */
	p->data_length = 0;
	p->data_fd = -1;

	/* Allocate and connect to IRQs. */
	p->irq = avr_alloc_irq (&avr->irq_pool, 0, IRQ_SD_CARD_COUNT, _irq_names);
	avr_irq_register_notify (p->irq + IRQ_SD_CARD_MOSI, _sd_card_mosi_pin_changed_cb, p);
	avr_irq_register_notify (p->irq + IRQ_SD_CARD_nSS, _sd_card_ss_pin_changed_cb, p);
}

/* Connect the IRQs of the SD card to the SPI bus of the microcontroller. */
void sd_card_attach (struct avr_t *avr, sd_card_t *p, uint32_t spi_irq_base, struct avr_irq_t *nss_irq)
{
	/* Connect MISO/MOSI. */
	avr_connect_irq (p->irq + IRQ_SD_CARD_MISO, avr_io_getirq (avr, spi_irq_base, SPI_IRQ_INPUT));
	avr_connect_irq (avr_io_getirq (avr, spi_irq_base, SPI_IRQ_OUTPUT), p->irq + IRQ_SD_CARD_MOSI);

	/* Connect the chip select. */
	avr_connect_irq (nss_irq, p->irq + IRQ_SD_CARD_nSS);
}

int sd_card_mount_file (struct avr_t *avr, sd_card_t *self, const char *filename, off_t image_size)
{
	int fd;
	void *mapped;
	int saved_errno;
	struct stat stat_buf;
	uint8_t locked = 0; /* boolean */
	off_t blocknr;
	uint8_t c_size_mult;
	uint16_t mult;
	uint16_t c_size;

	/* Open the specified disk image. */
	fd = open (filename, O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);

	if (fd == -1) {
		/* Error. */
		return errno;
	}

	/* Lock it for exclusive access. */
	if (flock (fd, LOCK_EX) == -1) {
		/* Error. */
		saved_errno = errno;
		goto error;
	}

	locked = 1;

	/* Check its size. If it's smaller than the requested size, expand it. Otherwise, ignore any excess size. */
	if (fstat (fd, &stat_buf) == -1) {
		/* Error. */
		saved_errno = errno;
		goto error;
	}

	if (stat_buf.st_size < image_size) {
		if (ftruncate (fd, image_size) == -1) {
			/* Error. */
			saved_errno = errno;
			goto error;
		}
	}

	/* Map it into memory. */
	mapped = mmap (NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (mapped == MAP_FAILED) {
		/* Error. */
		saved_errno = errno;
		goto error;
	}

	/* Success. */
	self->data = mapped;
	self->data_length = image_size;
	self->data_fd = fd;

	/* Update the C_SIZE field (number of sectors) in the CSD register. Reference for size calculations: JESD84-A44, Section 8.3, 'C_SIZE'. */
	blocknr = image_size / BLOCK_SIZE;
	c_size_mult = 4; /* arbitrarily chosen */
	mult = (1 << (c_size_mult + 2));
	c_size = (blocknr / mult) - 1;

	_sd_card_set_csd_c_size (self, c_size, c_size_mult);

	avr_raise_irq(self->irq + IRQ_SD_CARD_PRESENT,1);

	return 0;

error:
	/* Clean up after the error. */
	if (locked == 1) {
		flock (fd, LOCK_UN);
	}

	close (fd);

	return saved_errno;
}

int sd_card_unmount_file (struct avr_t *avr, sd_card_t *self)
{
	if (self->data == NULL) {
		/* No disk mounted. */
		return 0;
	}

	/* Synchronise changes. */
	msync (self->data, self->data_length, MS_SYNC | MS_INVALIDATE);

	/* Unlock the file. */
	flock (self->data_fd, LOCK_UN);

	/* munmap() and close. */
	munmap (self->data, self->data_length);
	close (self->data_fd);

	self->data = NULL;
	self->data_length = 0;
	self->data_fd = -1;

	_sd_card_set_csd_c_size (self, 0, 0);
	avr_raise_irq(self->irq + IRQ_SD_CARD_PRESENT,0);
	return 0;
}
