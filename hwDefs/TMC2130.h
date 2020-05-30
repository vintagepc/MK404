/*
	TMC2130.h - a trinamic driver simulator for Einsy.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

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


#ifndef __TMC2130_H__
#define __TMC2130_H__

#include "SPIPeripheral.h"

class TMC2130: public SPIPeripheral
{
    public:
        #define IRQPAIRS \
	        _IRQ(SPI_BYTE_IN,       "8<tmc2130.byte_in") \
	        _IRQ(SPI_BYTE_OUT,  	"8>tmc2130.byte_out") \
            _IRQ(SPI_COMMAND_IN,    "40<tmc2130.cmd_in") \
            _IRQ(SPI_CSEL,          "<tmc2130.cs_in") \
            _IRQ(STEP_IN,           "<tmc2130.step_in") \
            _IRQ(DIR_IN,            "<tmc2130.dir_in") \
            _IRQ(ENABLE_IN,         "<tmc2130.en_in") \
            _IRQ(DIAG_OUT,          ">tmc2130.diag_out") \
            _IRQ(MIN_OUT,           ">tmc2130.min_out") \
            _IRQ(POSITION_OUT,      ">tmc2130.pos_out") 
        #include "IRQHelper.h"

        struct TMC2130_cfg_t {
            TMC2130_cfg_t():bInverted(false),uiStepsPerMM(100),cAxis(' '),iMaxMM(200),fStartPos(10.0),bHasNoEndStops(false){};
            bool bInverted;
            uint16_t uiStepsPerMM;
            char cAxis;
            int16_t iMaxMM;
            float fStartPos;
            uint8_t uiDiagPin;
            bool bHasNoEndStops;
        };
        
        // Default constructor.
        TMC2130();

        // Sets the configuration to the provided values. (inversion, positions, etc)
        void SetConfig(TMC2130_cfg_t cfg);

        // Registers with SimAVR.
        void Init(avr_t *avr);

        // Draws a simple visual representation of the motor position.
        void Draw();

        // Draws the position value as a number, without position ticks.
        void Draw_Simple();

    private:

        // SPI handlers.
        uint8_t OnSPIIn(avr_irq_t *irq, uint32_t value) override;
        void OnCSELIn(avr_irq_t *irq, uint32_t value) override;

        // Input handlers.
        void OnDirIn(avr_irq_t *irq, uint32_t value);
        void OnStepIn(avr_irq_t *irq, uint32_t value);
        void OnEnableIn(avr_irq_t *irq, uint32_t value);

        // Standstill register handler.
        avr_cycle_count_t OnStandStillTimeout(avr_t *avr, avr_cycle_count_t when);
        avr_cycle_timer_t m_fcnStandstill = MAKE_C_TIMER_CALLBACK(TMC2130,OnStandStillTimeout);

        // Command processing
        void ProcessCommand();
        void CreateReply();

        void CheckDiagOut();

        bool m_bDir  = 0;
        bool m_bEnable = true; 
        
        TMC2130_cfg_t cfg;
        // Register definitions.
        typedef union tmc2130_cmd_t{
            tmc2130_cmd_t():all(0x0){}
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

        int32_t m_iCurStep = 0;
        uint32_t m_uiMaxPos = 0;
        float m_fCurPos = 0; // Tracks position in float for gl
        tmc2130_cmd_t m_cmdIn;
        tmc2130_cmd_t m_cmdProc;
        tmc2130_cmd_t m_cmdOut; // the previous data for output.
        tmc2130_registers_t m_regs;

};

#endif
