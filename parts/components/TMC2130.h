/*
	TMC2130.h - a trinamic driver simulator for Einsy.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

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

#include "BasePeripheral.h"    // for MAKE_C_TIMER_CALLBACK
#include "GLMotor.h"
#include "IScriptable.h"       // for IScriptable::LineStatus
#include "SPIPeripheral.h"     // for SPIPeripheral
#include "Scriptable.h"        // for Scriptable
#include "sim_avr.h"           // for avr_t
#include "sim_avr_types.h"     // for avr_cycle_count_t
#include "sim_cycle_timers.h"  // for avr_cycle_timer_t
#include "sim_irq.h"           // for avr_irq_t
#include <cstdint>            // for uint8_t, uint32_t, int32_t, uint16_t
#include <string>              // for string
#include <vector>              // for vector

class TMC2130: public SPIPeripheral, public Scriptable, public GLMotor
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
            _IRQ(POSITION_OUT,      ">tmc2130.pos_out") \
			_IRQ(STEP_POS_OUT, 		">tmc2130.step_out")
        #include "IRQHelper.h"

        using TMC2130_cfg_t = struct TMC2130_cfg_t
		{
            bool bInverted {false};
            uint16_t uiFullStepsPerMM {100U*16U}; // This is FULL steps per mm, at maximum (256us) resolution.
            int16_t iMaxMM {200};
            float fStartPos{ 10.0};
            bool bHasNoEndStops {false};
        };

        // Default constructor.
        explicit TMC2130(char cAxis = ' ');
		~TMC2130() override;

        // Sets the configuration to the provided values. (inversion, positions, etc)
        void SetConfig(TMC2130_cfg_t cfg);

		inline const TMC2130_cfg_t& GetConfig() { return cfg; }

        // Registers with SimAVR.
        void Init(avr_t *avr);

		void Reset() override;

	protected:
		Scriptable::LineStatus ProcessAction (unsigned int iAct, const std::vector<std::string> &vArgs) override;

    private:
		enum Actions
		{
			ActToggleStall,
			ActSetDiag,
			ActResetDiag,
		};

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

		// Stallguard DIAG handling
        void RaiseDiag(uint8_t value);
		void SetDiag();
		void ClearDiag();

        bool m_bDir  = false;

        TMC2130_cfg_t cfg;
        // Register definitions.
        using tmc2130_cmd_t = union tmc2130_cmd {
            uint64_t all :40;
            struct {
				uint32_t data :32; // 32 bits of data
				uint8_t address :7;
				uint8_t RW :1;
            } __attribute__((__packed__)) bitsIn;
            struct {
                uint32_t data :32; // 32 bits of data
                uint8_t reset_flag :1;
                uint8_t driver_error :1;
                uint8_t sg2 :1;
                uint8_t standstill :1;
                uint8_t :4; // unused
            }  __attribute__((__packed__)) bitsOut;
            uint8_t bytes[5] {0,0,0,0,0}; // Raw bytes as piped in/out by SPI.
        };

        // the internal programming registers.
        using tmc2130_registers_t =  union tmc2130_registers {
            uint32_t raw[128] {0}; // There are 128, 7-bit addressing.
            // Add fields for specific ones down the line as you see fit...
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
					uint16_t :14;
                }  __attribute__((__packed__)) GCONF;             // 0x00
                struct                 // 0x01
                {
                    uint8_t reset   :1;
                    uint8_t drv_err :1;
                    uint8_t uv_cp   :1;
					uint32_t :29; // unused
                }  __attribute__((__packed__)) GSTAT;
                uint32_t _unimplemented[2]; //0x02 - 0x03
                struct                 // 0x04
                {
					uint8_t         :6;  // unused
					uint8_t one     :1;  // always one
					uint8_t         :1;  // unused
					uint16_t        :16; // unused
					uint8_t version :8;
                }  __attribute__((__packed__)) IOIN;
                uint32_t _unimplemented2[103]; //0x05 - 0x6B
				struct                        //0x6C
				{
					uint32_t toff		:4;
					uint32_t hstrt		:3;
					uint32_t hend		:4;
					uint32_t fd3		:1;
					uint32_t disfdcc	:1;
					uint32_t rndtf		:1;
					uint32_t chm		:1;
					uint32_t tbl		:2;
					uint32_t vsense		:1;
					uint32_t vhighfs	:1;
					uint32_t vhighchm	:1;
					uint32_t sync		:4;
					uint32_t mres		:4;
					uint32_t intpol		:1;
					uint32_t dedge		:1;
					uint32_t diss2g		:1;
					uint32_t			:1;
				} __attribute__((__packed__)) CHOPCONF;
                uint32_t _unimplemented3[2]; //0x6D - 0x6E
                struct                       //0x6F
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
                }  __attribute__((__packed__)) DRV_STATUS;
            }defs;
        };

        tmc2130_cmd_t m_cmdIn {};
        tmc2130_cmd_t m_cmdProc {};
        tmc2130_cmd_t m_cmdOut {}; // the previous data for output.
        tmc2130_registers_t m_regs{};
		bool m_bStall = false;
		uint32_t m_uiStepIncrement = 1;

		// Position helpers
		float StepToPos(int32_t step) override;
		int32_t PosToStep(float step) override;

		inline uint32_t GetStepDivisor() { return 256U>>m_regs.defs.CHOPCONF.mres; }
};
