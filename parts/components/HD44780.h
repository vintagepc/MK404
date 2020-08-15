/*
	HD44780.h

	Original based on simavr hd44780.h (C) 2011 Michel Pollet <buserror@gmail.com>

    Rewritten for C++ in 2020, VintagePC <https://github.com/vintagepc/>

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

/*
 * This "Part" simulates the business end of a HD44780 LCD display
 * It supports from 8x1 to 20x4.
 *
 * It works both in 4 bit and 8 bit modes and supports a "quickie" method
 * of driving that is commonly used on AVR, namely
 * (msb) RW:E:RS:D7:D6:D5:D4 (lsb)
 *
 * + As usual, the "RW" pin is optional if you are willing to wait for the
 *   specific number of cycles as per the datasheet (37uS between operations)
 * + If you decide to use the RW pin, the "busy" flag is supported and will
 *   be automaticly cleared on the second read, to exercisee the code a bit.
 * + Cursor is supported, but not "display shift"
 * + The Character RAM is fully supported and custom characters are rendered.
 *
 * To interface this part, you can use the "INPUT" IRQs and hook them to the
 * simavr instance, if you use the RW pins or read back frim the display, you
 * can hook the data pins /back/ to the AVR too.
 *
 * The "part" also provides various IRQs that are there to be placed in a VCD file
 * to show what is sent, and some of the internal status.
 *
 * This part has been tested extensively through use on the MK404 printer simulator
 * and it works on stock out of the box firmware.
 */

#pragma once

#include "Scriptable.h"        // for Scriptable
#include "BasePeripheral.h"    // for MAKE_C_TIMER_CALLBACK, BasePeripheral
#include "IScriptable.h"       // for ArgType, ArgType::Int, ArgType::String
#include "gsl-lite.hpp"
#include "sim_avr.h"           // for avr_t
#include "sim_avr_types.h"     // for avr_cycle_count_t
#include "sim_cycle_timers.h"  // for avr_cycle_timer_t
#include "sim_irq.h"           // for avr_irq_t
#include <atomic>
#include <cstdint>            // for uint8_t, uint16_t, uint32_t
#include <mutex>
#include <string>              // for string
#include <vector>              // for vector

class HD44780:public BasePeripheral, public Scriptable
{
	public:
		#define IRQPAIRS _IRQ(ALL,"7=hd44780.pins") \
			_IRQ(RS,"<hd44780.RS") \
			_IRQ(RW,"<hd44780.RW") \
			_IRQ(E,"<hd44780.E") \
			_IRQ(D0,"=hd44780.D0") \
			_IRQ(D1,"=hd44780.D1") \
			_IRQ(D2,"=hd44780.D2") \
			_IRQ(D3,"=hd44780.D3") \
			_IRQ(D4,"=hd44780.D4") \
			_IRQ(D5,"=hd44780.D5") \
			_IRQ(D6,"=hd44780.D6") \
			_IRQ(D7,"=hd44780.D7") \
			_IRQ(BUSY,">hd44780.BUSY") \
			_IRQ(ADDR,"7>hd44780.ADDR") \
			_IRQ(DATA_IN,"8>hd44780.DATA_IN") \
			_IRQ(DATA_OUT,"8>hd44780.DATA_OUT") \
			_IRQ(BRIGHTNESS_IN,"=hd44780.BRIGHTNESS_IN") \
            _IRQ(BRIGHTNESS_PWM_IN,"8<hd44780.BRIGHTNESS_PWM_IN")
		#include "IRQHelper.h"

		// Makes a display with the given dimensions.
		HD44780(uint8_t width = 20, uint8_t height = 4):Scriptable("LCD"),m_uiHeight(height),m_uiWidth(width)
		{
			m_cgRam.resize(64,' ');
			m_vRam.resize(104,' ');
			m_lineOffsets[2] += width;
			m_lineOffsets[3] += width;
			string strBlnk;
			strBlnk.assign(width,' ');
			for (int i=0; i<height; i++)
				m_vLines.push_back(strBlnk);

			RegisterActionAndMenu("Desync","Simulates data corruption by desyncing the 4-bit mode",ActDesync);
			RegisterAction("WaitForText","Waits for a given string to appear anywhere on the specified line. A line value of -1 means any line.",ActWaitForText,{ArgType::String,ArgType::Int});
		};

		// Registers IRQs with SimAVR.
		void Init(avr_t *avr);

		// Returns height and width.
        uint8_t GetWidth() { return m_uiWidth;}
        uint8_t GetHeight() { return m_uiHeight;}

    protected:
    // The GL draw accesses these:
		atomic_uint8_t m_uiHeight = {4};				// width and height of the LCD
        atomic_uint8_t	m_uiWidth = {20};
		std::vector<uint8_t>  m_vRam;
        std::vector<uint8_t>  m_cgRam;

		LineStatus ProcessAction(unsigned int iAction, const vector<string> &args) override;

		inline void ToggleFlag(uint16_t bit)
		{
			m_flags ^= (1<<bit);
		}

		inline bool SetFlag(uint16_t bit, uint8_t uiVal)
		{
			int old = m_flags &  (1 << bit);
			m_flags = (m_flags & ~(1 << bit)) | (uiVal ? (1 << bit) : 0);
			return old != 0;
		}

		inline bool GetFlag(uint16_t bit)
		{
			return (m_flags &  (1 << bit)) != 0;
		}


		enum {
			HD44780_FLAG_F = 0,         // 1: 5x10 Font, 0: 5x7 Font
			HD44780_FLAG_N,             // 1: 2/4-lines Display, 0: 1-line Display,
			HD44780_FLAG_D_L,           // 1: 8-Bit Interface, 0: 4-Bit Interface
			HD44780_FLAG_R_L,           // 1: Shift right, 0: shift left
			HD44780_FLAG_S_C,           // 1: Display shift, 0: Cursor move
			HD44780_FLAG_B,             // 1: Cursor Blink
			HD44780_FLAG_C,             // 1: Cursor on
			HD44780_FLAG_D,             // 1: Set Entire Display memory (for clear)
			HD44780_FLAG_S,             // 1: Follow display shift
			HD44780_FLAG_I_D,			// 1: Increment, 0: Decrement

			/*
			* Internal flags, not HD44780
			*/
			HD44780_FLAG_LOWNIBBLE,		// 1: 4 bits mode, write/read low nibble
			HD44780_FLAG_BUSY,			// 1: Busy between instruction, 0: ready
			HD44780_FLAG_REENTRANT,		// 1: Do not update pins

			HD44780_FLAG_DIRTY,			// 1: needs redisplay...
		};

		std::vector<uint8_t> m_lineOffsets = {0, 0x40, 0, 0x40};

		mutex m_lock; // Needed for GL thread access to v/cgRAM

	private:
		enum Actions
		{
			ActDesync,
			ActWaitForText
		};

        void ResetCursor();
        void ClearScreen();

        void OnPinChanged(avr_irq_t *irq, uint32_t value);
        avr_cycle_count_t OnEPinChanged(avr_t *avr, avr_cycle_count_t value);

		avr_cycle_timer_t m_fcnEPinChanged = MAKE_C_TIMER_CALLBACK(HD44780,OnEPinChanged);

        void IncrementCursor();
        void IncrementCGRAMCursor();

        uint32_t OnDataReady();
        uint32_t OnCmdReady();

        uint32_t ProcessWrite();
        uint32_t ProcessRead();

        avr_cycle_count_t OnBusyTimeout(avr_t *avr, avr_cycle_count_t when);

		avr_cycle_timer_t m_fcnBusy = MAKE_C_TIMER_CALLBACK(HD44780,OnBusyTimeout);

        uint16_t m_uiCursor = 0, m_uiCGCursor = 0;			// offset in vram
        bool m_bInCGRAM = false;

        uint16_t m_uiPinState  = 0;			// 'actual' LCd data pins (IRQ bit field)
        uint8_t	 m_uiDataPins = 0;			// composite of 4 high bits, or 8 bits
        uint8_t  m_uiReadPins = 0;
        volatile uint16_t m_flags = 0;				// LCD flags ( HD44780_FLAG_*)

		vector<string> m_vLines;

		uint8_t m_uiLineChg = 0;



};
