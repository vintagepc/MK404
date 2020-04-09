/*
	An SPI flash emulator for the external language flash
 */

#ifndef __TMC2130_H__
#define __TMC2130_H__

#include "sim_irq.h"

/*
 * this one is quite fun, it simulated a 74HC595 shift register
 * driven by an SPI signal.
 * For the interest of the simulation, they can be chained, but 
 * for practicality sake the shift register is kept 32 bits
 * wide so it acts as 4 of them "daisy chained" already. 
 */
enum {
	IRQ_TMC2130_SPI_BYTE_IN = 0,	// if hooked to a byte based SPI IRQ
	IRQ_TMC2130_SPI_BYTE_OUT,		// to chain them !!
    IRQ_TMC2130_SPI_COMMAND_IN, // full 40-bit command
    IRQ_TMC2130_SPI_CSEL,
    //IRQ_TMC2130_STEP_IN,
    //IRQ_TMC2130_DIR_IN,
    //IRQ_TMC2130_COIL_A1_OUT,
    //IRQ_TMC2130_COIL_A2_OUT,
    //IRQ_TMC2130_COIL_B1_OUT,
    //IRQ_TMC2130_COIL_B2_OUT,
	IRQ_TMC2130_COUNT
};

typedef union tmc2130_flags_t{
    uint8_t flags;
    struct 
    {
        uint8_t selected : 1;
    } bits;
} tmc2130_flags_t;

typedef union tmc2130_cmdIn_t{
    uint8_t bytes[5]; // Raw bytes as piped in by SPI.
    struct {
        uint8_t RW :1;
        uint8_t address :7;
        unsigned long data :32; // 32 bits of data
    } bits
} tmc2130_cmdIn_t;

typedef union tmc2130_cmdOut_t{
    uint8_t bytes[5]; // Raw bytes as piped in by SPI.
    struct {
        uint8_t :5; // unused
        uint8_t standstill :1;
        uint8_t sg2 :1;
        uint8_t driver_error :1;
        uint8_t reset_flag :1;
        unsigned long data; // 32 bits of data
    } bits
} tmc2130_cmdOut_t;

typedef struct tmc2130_t {
	avr_irq_t *	irq;		// irq list
    char axis; // Useful for print debugging.
	tmc2130_flags_t flags;
	// TODO...
} tmc2130_t;

void
tmc2130_init(
		struct avr_t * avr,
		tmc2130_t *p, char axis);

#endif
