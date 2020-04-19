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
    IRQ_TMC2130_STEP_IN,
    IRQ_TMC2130_DIR_IN,
    IRQ_TMC2130_ENABLE_IN,
    IRQ_TMC2130_DIAG_OUT,
    IRQ_TMC2130_MIN_OUT,
    IRQ_TMC2130_POSITION_OUT,
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
        uint8_t dir : 1;
        uint8_t enable :1;
        uint8_t inverted :1;
    } bits;
} tmc2130_flags_t;

typedef union tmc2130_cmd_t{
    uint64_t all :40;
     struct {
        unsigned long data :32; // 32 bits of data
        uint8_t address :7;
        uint8_t RW :1;
    } bitsIn;
    struct {
        unsigned long data :32; // 32 bits of data
        uint8_t reset_flag :1;
        uint8_t driver_error :1;
        uint8_t sg2 :1;
        uint8_t standstill :1;
        uint8_t :5; // unused
    } bitsOut;
    uint8_t bytes[5]; // Raw bytes as piped in/out by SPI.
} tmc2130_cmd_t;

// the internal programming registers.
typedef union
{
    uint32_t raw[128]; // There are 128, 7-bit addressing.
    // TODO: add fields for specific ones down the line...
    struct {
        struct {
            uint8_t I_scale_analog  :1;
            uint8_t internal_Rsense :1;
            uint8_t en_pwm_mode :1;
            uint8_t enc_communication   :1;
            uint8_t shaft   :1;
            uint8_t diag0_error :1;
            uint8_t diag0_optw  :1;
            uint8_t diag0_stall :1;
            uint8_t diag1_stall :1;
            uint8_t diag1_index :1;
            uint8_t diag1_onstate   :1;
            uint8_t diag1_steps_skipped :1;
            uint8_t diag0_int_pushpull  :1;
            uint8_t diag1_int_pushpull  :1;
            uint8_t small_hysteresis    :1;
            uint8_t stop_enable :1;
            uint8_t direct_mode         :1;
        } GCONF;             // 0x00
        struct                 // 0x01
        {
            uint8_t reset   :1;
            uint8_t drv_err :1;
            uint8_t uv_cp   :1;
        } GSTAT;
        uint32_t _unimplemented[110];   // 0x02 - 0x6E
        struct                      //0x6F
        {
            uint16_t SG_RESULT   :10;
            uint8_t             :5;
            uint8_t fsactive    :1;
            uint8_t CS_ACTUAL   :5;
            uint8_t             :3;
            uint8_t stallGuard  :1;
            uint8_t ot          :1;
            uint8_t otpw        :1;
            uint8_t sg2a        :1;
            uint8_t sg2b        :1;
            uint8_t ola         :1;
            uint8_t olb         :1;
            uint8_t stst        :1;
        }DRV_STATUS;
    }defs;
} tmc2130_registers_t;

typedef struct tmc2130_t {
	avr_irq_t *	irq;		// irq list
    char axis; // Useful for print debugging.
	tmc2130_flags_t flags;
    uint16_t iStepsPerMM;
    uint32_t iMaxPos;
    int32_t iCurStep;
    float fCurPos; // Tracks position in float for gl
    tmc2130_cmd_t cmdIn;
    tmc2130_cmd_t cmdProc;
    tmc2130_cmd_t cmdOut; // the previous data for output.
    tmc2130_registers_t regs;
	// TODO...
} tmc2130_t;

void tmc2130_draw_glut(tmc2130_t *p);
void tmc2130_draw_position_glut(tmc2130_t *p);

void
tmc2130_init(
		struct avr_t * avr,
		tmc2130_t *p, char axis, uint8_t iDiagADC);

#endif
