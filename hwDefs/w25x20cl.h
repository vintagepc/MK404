/*
	An SPI flash emulator for the external language flash
 */

#ifndef __W25X20CL_H__
#define __W25X20CL_H__

#include "sim_irq.h"

/*
 * this one is quite fun, it simulated a 74HC595 shift register
 * driven by an SPI signal.
 * For the interest of the simulation, they can be chained, but 
 * for practicality sake the shift register is kept 32 bits
 * wide so it acts as 4 of them "daisy chained" already. 
 */

#define W25X20CL_TOTAL_SIZE 262144
#define W25X20CL_PAGE_SIZE 256
#define W25X20CL_SECTOR_SIZE 4096
#define W25X20CL_BLOCK32_SIZE 32768
#define W25X20CL_BLOCK64_SIZE 65536


enum {
	IRQ_W25X20CL_SPI_BYTE_IN = 0,	// if hooked to a byte based SPI IRQ
	IRQ_W25X20CL_SPI_BYTE_OUT,		// to chain them !!
	IRQ_W25X20CL_SPI_CSEL,
	IRQ_W25X20CL_COUNT
};

enum w25x20cl_states{
	W25X20CL_STATE_IDLE = 0, //when CS is HIGH
	W25X20CL_STATE_LOADING,
	W25X20CL_STATE_RUNNING,
};

typedef struct w25x20cl_t {
	avr_irq_t *	irq;		// irq list
	uint8_t flash[W25X20CL_TOTAL_SIZE];
	uint8_t pageBuffer[W25X20CL_PAGE_SIZE];
	uint8_t cmdIn[5];
	uint8_t rxCnt;
	uint8_t cmdOut;
	uint8_t command;
	uint32_t address;
	uint64_t UID;
	union
	{
		uint8_t byte;
		struct
		{
			uint8_t BUSY :1;
			uint8_t WEL :1;
			uint8_t BP :2;
			uint8_t RES4 :1;
			uint8_t TB :1;
			uint8_t RES6 :1;
			uint8_t SRP :1;
		} bits;
	} status_register;
	int state;
	int xflash_fd;
	char filepath[1024];
	// TODO...
} w25x20cl_t;

void w25x20cl_init(
		struct avr_t * avr,
		w25x20cl_t *p,
		avr_irq_t *irqCS);

int w25x20cl_load(
		const char* path,
		w25x20cl_t *p);

void w25x20cl_save(
		const char* path, 
		w25x20cl_t *p);

#endif
