/*
	sd_card.h - an SD card SPI simulator.

	Original  Copyright (C) Philip Withnall 2012 <philip@tecnocode.co.uk> as part of Brewing Logger

	Modified for use with MK3SIM in 2020 by leptun <https://github.com/leptun/>

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

#ifndef _SD_CARD_H
#define _SD_CARD_H

/**
 * \brief IRQs for the SD card.
 *
 * Identifiers for the SD card's IRQs.
 */
enum {
	IRQ_SD_CARD_MOSI = 0,	/**< Byte input port. Serial; normally low. */
	IRQ_SD_CARD_MISO,	/**< Byte output port. Serial; normally low. */
	IRQ_SD_CARD_nSS,	/**< Slave select port. Input; active low. */
	IRQ_SD_CARD_PRESENT, // Card detect out, toggles with mount/unmount of image.
	IRQ_SD_CARD_COUNT
};

/* TODO */
typedef enum {
	SD_CARD_IDLE,
	SD_CARD_COMMAND_REQUEST,
	SD_CARD_COMMAND_RESPONSE,
	SD_CARD_DATA_READ_TOKEN,
	SD_CARD_DATA_READ,
	SD_CARD_DATA_READ_CRC,
	SD_CARD_DATA_WRITE_TOKEN,
	SD_CARD_DATA_WRITE,
	SD_CARD_DATA_WRITE_CRC,
} sd_card_state_t;

/**
 * \brief SD card peripheral.
 *
 * Data structure holding the state of a simulated SD card peripheral, connected over SPI. Only \c irq should be accessed by client code.
 */
typedef struct sd_card_t {
	avr_irq_t *irq;
	struct avr_t *avr;
	char filepath[1024];

	/* Internal state. */
	sd_card_state_t state;
	struct {
		uint8_t command;
		uint8_t params[4];
		uint8_t checksum;
	} command_header;
	struct {
		uint8_t data[5];
		uint8_t length; /* number of bytes of data which are valid */
	} command_response;
	uint8_t command_index;
	uint8_t selected : 1; /* 1 iff the chip is selected */

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
	uint32_t ocr; /* operation conditions register (OCR) */
	uint8_t csd[16]; /* card-specific data (CSD) register */

	uint16_t CRC;

	/* Card data. */
	uint8_t *data; /* mmap()ed data */
	off_t data_length;
	int data_fd;
} sd_card_t;

/**
 * \brief Initialise SD card peripheral.
 *
 * Initialise the passed-in \c sd_card_t structure, assuming it's already been allocated. This creates IRQs, which may be accessed using
 * \c sd_card_t->irq after this call returns.
 */
void sd_card_init (struct avr_t *avr, sd_card_t *p);

/* TODO */
void sd_card_attach (struct avr_t *avr, sd_card_t *p, uint32_t spi_irq_base, struct avr_irq_t *nss_irq);

/* TODO */
int sd_card_mount_file (struct avr_t *avr, sd_card_t *self, const char *filename, off_t image_size);

/* TODO */
int sd_card_unmount_file (struct avr_t *avr, sd_card_t *self);

#endif /* !_SD_CARD_H */
