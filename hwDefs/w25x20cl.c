/*
	hc595.c

	This defines a sample for a very simple "peripheral" 
	that can talk to an AVR core.
	It is in fact a bit more involved than strictly necessary,
	but is made to demonstrante a few useful features that are
	easy to use.
	
	Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

 	This file is part of simavr.

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "sim_avr.h"
#include "avr_spi.h"
#include "w25x20cl.h"

#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif


#define _MFRID             0xEF
#define _DEVID             0x11

#define _CMD_ENABLE_WR     0x06
// #define _CMD_ENABLE_WR_VSR 0x50
#define _CMD_DISABLE_WR    0x04
#define _CMD_RD_STATUS_REG 0x05
#define _CMD_WR_STATUS_REG 0x01
#define _CMD_RD_DATA       0x03
// #define _CMD_RD_FAST       0x0b
// #define _CMD_RD_FAST_D_O   0x3b
// #define _CMD_RD_FAST_D_IO  0xbb
#define _CMD_PAGE_PROGRAM  0x02
#define _CMD_SECTOR_ERASE  0x20
#define _CMD_BLOCK32_ERASE 0x52
#define _CMD_BLOCK64_ERASE 0xd8
#define _CMD_CHIP_ERASE    0xc7
// #define _CMD_CHIP_ERASE2   0x60
// #define _CMD_PWR_DOWN      0xb9
// #define _CMD_PWR_DOWN_REL  0xab
#define _CMD_MFRID_DEVID   0x90
// #define _CMD_MFRID_DEVID_D 0x92
// #define _CMD_JEDEC_ID      0x9f
#define _CMD_RD_UID        0x4b

#define W25X20CL_STATUS_BUSY   0x01
#define W25X20CL_STATUS_WEL    0x02
#define W25X20CL_STATUS_BP0    0x04
#define W25X20CL_STATUS_BP1    0x08
#define W25X20CL_STATUS_TB     0x20
#define W25X20CL_STATUS_SRP    0x80

#define SPI_SEND() do{\
avr_raise_irq(this->irq + IRQ_W25X20CL_SPI_BYTE_OUT, this->cmdOut);\
TRACE(printf("W25X20CL: Clocking out %02x\n", this->cmdOut));\
}while (0)

/*
 * called when a SPI byte is sent
 */
static void w25x20cl_spi_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	w25x20cl_t* this = (w25x20cl_t*)param;
	switch (this->state)
	{
		case W25X20CL_STATE_LOADING:
		{
			if (this->rxCnt >= sizeof(this->cmdIn))
			{
				printf("w25x20cl_t: error: command too long: ");
				for (int i = 0; i < sizeof(this->cmdIn); i++)
				{
					printf("%02x, ", this->cmdIn[i]);
				}
				printf("\n");
				break;
			}
			this->cmdIn[this->rxCnt] = value;
			this->rxCnt++;
			
			
			// Check for a loaded instruction in the cmdIn buffer
			this->command = this->cmdIn[0];
			switch(this->command)
			{
				case _CMD_RD_DATA:
				case _CMD_MFRID_DEVID:
				{
					if (this->rxCnt == 4)
					{
						this->address = 0;
						for (uint  i = 0; i < 3; i++)
						{
							this->address <<= 8;
							this->address |= this->cmdIn[i + 1];
						}
						this->address %= W25X20CL_TOTAL_SIZE;
						this->state = W25X20CL_STATE_RUNNING;
					}
				} break;
				
				default:
				{
				printf("w25x20cl_t: error: unknown command: ");
				for (int i = 0; i < this->rxCnt; i++)
				{
					printf("%02x, ", this->cmdIn[i]);
				}
				printf("\n");
				} break;
			}
			
		} break;
		case W25X20CL_STATE_RUNNING:
		{
			TRACE(printf("w25x20cl_t: command:%02x, addr:%05x\n", this->command, this->address));
			switch (this->command)
			{
				case _CMD_MFRID_DEVID:
				{
					this->cmdOut = (this->address % 2)?_DEVID:_MFRID;
					this->address++;
					this->address %= W25X20CL_TOTAL_SIZE;
					SPI_SEND();
				} break;
				case _CMD_RD_DATA:
				{
					this->cmdOut = this->flash[this->address];
					this->address++;
					this->address %= W25X20CL_TOTAL_SIZE;
					SPI_SEND();
				} break;
			}
		}
		case W25X20CL_STATE_IDLE:
		default:
		{
			return;
		} break;
	}
	TRACE(printf("w25x20cl_t: byte received: %02x\n",value));
}

// Called when CSEL changes.
static void w25x20cl_csel_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	w25x20cl_t* this = (w25x20cl_t*)param;
	TRACE(printf("w25x20cl_t: CSEL changed to %02x\n",value));
	if (value == 0)
	{
		this->state = W25X20CL_STATE_LOADING;
		memset(this->cmdIn, 0, sizeof(this->cmdIn));
		this->rxCnt = 0;
		this->cmdOut = 0;
		this->command = 0;
		this->address = 0;
		this->page_pointer = 0;
	}
	else
	{
		this->state = W25X20CL_STATE_IDLE;
		this->cmdOut = 0xFF;
		SPI_SEND();
	}
}

static const char * irq_names[IRQ_W25X20CL_COUNT] = {
		[IRQ_W25X20CL_SPI_BYTE_IN] = "8<w25x20cl.in",
		[IRQ_W25X20CL_SPI_BYTE_OUT] = "8>w25x20cl.chain",
};

void w25x20cl_init(
		struct avr_t * avr,
		w25x20cl_t *p)
{
	p->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_W25X20CL_COUNT, irq_names);
	
	avr_connect_irq(avr_io_getirq(avr,AVR_IOCTL_SPI_GETIRQ(0),SPI_IRQ_OUTPUT),p->irq + IRQ_W25X20CL_SPI_BYTE_IN);
	avr_connect_irq(p->irq + IRQ_W25X20CL_SPI_BYTE_OUT,avr_io_getirq(avr,AVR_IOCTL_SPI_GETIRQ(0),SPI_IRQ_INPUT));
	
	avr_irq_register_notify(p->irq + IRQ_W25X20CL_SPI_BYTE_IN, w25x20cl_spi_in_hook, p);
	avr_irq_register_notify(p->irq + IRQ_W25X20CL_SPI_CSEL, w25x20cl_csel_in_hook, p);
}

int w25x20cl_load(
		const char* path,
		w25x20cl_t *p)
{
	w25x20cl_t* this = (w25x20cl_t*)p;
	// Now deal with the external flash. Can't do this in special_init, it's not allocated yet then.
	int fd = open(path, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		perror(path);
		exit(1);
	}
	printf("Loading %u bytes of XFLASH\n", W25X20CL_TOTAL_SIZE);
	(void)ftruncate(fd, W25X20CL_TOTAL_SIZE + 1);
	uint8_t *buffer = malloc(W25X20CL_TOTAL_SIZE + 1);
	ssize_t r = read(fd, buffer, W25X20CL_TOTAL_SIZE + 1);
	printf("Read %d bytes\n", (int)r);
	if (r !=  W25X20CL_TOTAL_SIZE + 1) {
		fprintf(stderr, "unable to load XFLASH\n");
		perror(path);
		exit(1);
	}
	uint8_t bEmpty = 1;
	for (int i = 0; i < W25X20CL_TOTAL_SIZE + 1; i++)
	{
		bEmpty &= buffer[i] == 0;
	}
	if (!bEmpty) // If the file was newly created (all null) this leaves the internal eeprom as full of 0xFFs.
		memcpy(this->flash, buffer, W25X20CL_TOTAL_SIZE + 1);

	free(buffer);
	return fd;
}

void w25x20cl_save(
		const char* path, 
		w25x20cl_t *p)
{
	w25x20cl_t* this = (w25x20cl_t*)p;
	// Also write out the xflash contents:
	lseek(this->xflash_fd, SEEK_SET, 0);
	ssize_t r = write(this->xflash_fd, this->flash, W25X20CL_TOTAL_SIZE + 1);
	if (r != W25X20CL_TOTAL_SIZE + 1) {
		fprintf(stderr, "unable to write xflash memory\n");
		perror(path);
	}
	close(this->xflash_fd);
}

