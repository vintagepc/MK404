/* HD44780.cpp

	Original based on simavr hd44780.c (C) 2011 Michel Pollet <buserror@gmail.com>

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

#include "HD44780.h"
#include "Scriptable.h"      // for Scriptable
#include "TelemetryHost.h"
#include "gsl-lite.hpp"
#include <iostream>
#include <memory>


//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif

		// Makes a display with the given dimensions.
HD44780::HD44780(uint8_t width, uint8_t height):Scriptable("LCD"),m_uiHeight(height),m_uiWidth(width)
{
	m_lineOffsets[2] += width;
	m_lineOffsets[3] += width;
	std::string strBlnk;
	strBlnk.assign(width,' ');
	for (int i=0; i<height; i++)
	{
		m_vLines.push_back(strBlnk);
	}
	RegisterActionAndMenu("Desync","Simulates data corruption by desyncing the 4-bit mode",ActDesync);
	RegisterAction("WaitForText","Waits for a given string to appear anywhere on the specified line. A line value of -1 means any line.",ActWaitForText,{ArgType::String,ArgType::Int});
	RegisterAction("CheckCGRAM","Checks if the CGRAM address matches the value. (value, addr)",ActCheckCGRAM,{ArgType::Int,ArgType::Int});
};

void HD44780::ResetCursor()
{
	m_uiCursor = m_uiCGCursor = 0;
	m_bInCGRAM = false;
	TRACE(printf("cursor reset\n"));
	SetFlag(HD44780_FLAG_DIRTY, 1);
	RaiseIRQ(ADDR, m_uiCursor);
}

void HD44780::ClearScreen()
{
	{
		std::lock_guard<std::mutex> lock(m_lock);
    	for (auto &c : m_vRam)
		{
			c = ' ';
		}
	}
	SetFlag(HD44780_FLAG_DIRTY, 1);
	RaiseIRQ(ADDR, m_uiCursor);
	for (int i=0; i<m_uiHeight; i++)
	{
		m_vLines.at(i).assign(m_uiWidth,' ');
	}
	m_uiLineChg = 0xFF;
}

/*
 * This is called when the delay between operation is triggered
 * without the AVR firmware 'reading' the status byte. It
 * automatically clears the BUSY flag for the next command
 */
avr_cycle_count_t HD44780::OnBusyTimeout(struct avr_t *,avr_cycle_count_t)
{
	SetFlag(HD44780_FLAG_BUSY, 0);
	RaiseIRQ(BUSY, 0);
	return 0;
}

Scriptable::LineStatus HD44780::ProcessAction(unsigned int iAction, const std::vector<std::string> &vArgs)
{
	switch (iAction)
	{
		case ActDesync:
			ToggleFlag(HD44780_FLAG_LOWNIBBLE);
			return LineStatus::Finished;
		case ActCheckCGRAM:
		{
			int iAddr = stoi(vArgs.at(1));
			if (iAddr<0 || iAddr>63)
			{
				return IssueLineError(std::string("ADDR") + std::to_string(iAddr) + " is out of range [0,63]");
			}
			if (m_cgRam[iAddr] == stoi(vArgs.at(0)))
			{
				return LineStatus::Finished;
			}
			else
			{
				return LineStatus::Timeout;
			}
		}
		case ActWaitForText:
			int iLine = stoi(vArgs.at(1));
			uint8_t uiLnChk = iLine<0 ? 0xFF : 1U<<gsl::narrow<uint8_t>(iLine);
			if (!(uiLnChk & m_uiLineChg)) // NO changes to check against.
			{
				return LineStatus::Waiting;
			}

			if (iLine>=m_uiHeight || iLine<-1)
			{
				return IssueLineError(std::string("Line index ") + std::to_string(iLine) + " is out of range [-1," + std::to_string (m_uiHeight) + "]");
			}

			bool bResult = false;
			if (iLine<0)
			{
				for (int i=0; i<m_uiHeight; i++)
				{
					bResult |= m_vLines.at(i).find(vArgs.at(0))!=std::string::npos;
					if (bResult) break;
				}
			}
			else
			{
				bResult = m_vLines.at(iLine).find(vArgs.at(0))!=std::string::npos;
			}
			m_uiLineChg^= iLine<0 ? 0xFF : 1U<<gsl::narrow<uint8_t>(iLine); // Reset line change tracking.
			return bResult ? LineStatus::Finished : LineStatus::Waiting;
	}
	return LineStatus::Unhandled;
}


void HD44780::IncrementCursor()
{

	if (GetFlag(HD44780_FLAG_I_D)) {
		TRACE(printf("Cursor++ (%02x)\n",m_uiCursor));
		if (m_uiCursor == 0x67) // end of display.
		{
			m_uiCursor = 0x00; // wrap.
		}
		else if (m_uiCursor == 0x27)
		{
			m_uiCursor = 0x40;
		}
		else
		{
			m_uiCursor++;
		}
	}
	else
	 {
		TRACE(printf("Cursor--\n"));
		if (m_uiCursor == 0x00)
		{
			m_uiCursor = 0x67;
		}
		else if (m_uiCursor == 0x40)
		{
			m_uiCursor = 0x27;
		}
		else
		{
			m_uiCursor--;
		}

		//SetFlag(HD44780_FLAG_DIRTY, 1);
		//avr_raise_irq(b->irq + ADDR, m_uiCursor);
	}
}

// Nudge the CGRAM cursor value.
void HD44780::IncrementCGRAMCursor()
{
	if (GetFlag(HD44780_FLAG_I_D))
	{
		if (m_uiCGCursor==63)
		{
			m_uiCGCursor = 0;
		}
		else
		{
			m_uiCGCursor++;
		}
	}
	else
	{
		if (m_uiCGCursor==0)
		{
			m_uiCGCursor = 63;
		}
		else
		{
			m_uiCGCursor--;
		}
	}
}

/*
 * current data byte is ready in m_uiDataPins
 */
uint32_t HD44780::OnDataReady()
{
	uint32_t delay = 37; // uS
	if (m_bInCGRAM)
	{
		std::lock_guard<std::mutex> lock(m_lock);
		m_cgRam[m_uiCGCursor] = m_uiDataPins;
		TRACE(printf("hd44780_write_data %02x to CGRAM %02x\n",m_uiDataPins,m_uiCGCursor));
		IncrementCGRAMCursor();
	}
	else
	{
		{
			std::lock_guard<std::mutex> lock(m_lock);
			m_vRam[m_uiCursor] = m_uiDataPins;
		}

		for (unsigned int i=0; i<m_uiHeight; i++) // Flag line change for search performance.
		{
			if (m_uiCursor>= m_lineOffsets.at(i) && m_uiCursor< (m_lineOffsets.at(i) + m_uiWidth))
			{
				int iPos =m_uiCursor - m_lineOffsets.at(i);
				std::string &line = m_vLines[i];
				line[iPos] = m_uiDataPins;
				m_uiLineChg |= 1U<<i;
			}
		}
		TRACE(printf("hd44780_write_data %02x (%c) to %02x\n", m_uiDataPins, m_uiDataPins, m_uiCursor));
		if (GetFlag(HD44780_FLAG_S_C)) {	// display shift ?
			std::cout << "Display shift requested. Not implemented, sorry!\n";
		} else {
			IncrementCursor();
		}

	}
    SetFlag(HD44780_FLAG_DIRTY, 1);
	return delay;
}

/*
 * current command is ready in m_uiDataPins
 */
uint32_t HD44780::OnCmdReady()
{
	uint32_t delay = 37; // uS
	unsigned int top = 7;	// get highest bit set'm
	while (top)
	{
		if (m_uiDataPins & (1U << top))
		{
			break;
		}
		else
		{
			top--;
		}
	}
	TRACE(printf("hd44780_write_command %02x (top: %u)\n", m_uiDataPins,top));
	switch (top) {
		// Set	DDRAM address
		case 7:		// 1 ADD ADD ADD ADD ADD ADD ADD
			m_uiCursor = m_uiDataPins & 0x7fU;
			m_bInCGRAM = false;
			break;
		// Set	CGRAM address
		case 6:		// 0 1 ADD ADD ADD ADD ADD ADD ADD
			TRACE(printf("cgram enter\n"));
			m_bInCGRAM = true;
			m_uiCGCursor = (m_uiDataPins & 0x3fU);
			break;
		// Function	set
		case 5:		// 0 0 1 DL N F x x
		{
			int four = !GetFlag(HD44780_FLAG_D_L);
			SetFlag(HD44780_FLAG_D_L, m_uiDataPins & 16U);
			SetFlag(HD44780_FLAG_N, m_uiDataPins & 8U);
			SetFlag(HD44780_FLAG_F, m_uiDataPins & 4U);
			if (!four && !GetFlag(HD44780_FLAG_D_L)) {
				std::cout << static_cast<const char*>(__FUNCTION__) << "activating 4-bit mode" << '\n';
				SetFlag(HD44780_FLAG_LOWNIBBLE, 0);
			}
		}
			break;
		// Cursor display shift
		case 4:		// 0 0 0 1 S/C R/L x x
			SetFlag(HD44780_FLAG_R_L, m_uiDataPins & 4U);
			SetFlag(HD44780_FLAG_S_C, m_uiDataPins & 8U);
			break;
		// Display on/off control
		case 3:		// 0 0 0 0 1 D C B
			SetFlag(HD44780_FLAG_D, m_uiDataPins & 4U);
			SetFlag(HD44780_FLAG_C, m_uiDataPins & 2U);
			SetFlag(HD44780_FLAG_B, m_uiDataPins & 1U);
			SetFlag(HD44780_FLAG_DIRTY, 1);
			break;
		// Entry mode set
		case 2:		// 0 0 0 0 0 1 I/D S
			SetFlag(HD44780_FLAG_I_D, m_uiDataPins & 2U);
			SetFlag(HD44780_FLAG_S, m_uiDataPins & 1U);
			break;
		// Return home
		case 1:		// 0 0 0 0 0 0 1 x
			ResetCursor();
			delay = 1520;
			break;
		// Clear display
		case 0:		// 0 0 0 0 0 0 0 1
			ClearScreen();
			ResetCursor();
			delay = 1520;
			break;
	}
	return delay;
}

/*
 * the E pin went low, and it's a write
 */
uint32_t HD44780::ProcessWrite()
{
	if (GetFlag(HD44780_FLAG_BUSY))
	{
		std::cout << static_cast<const char*>(__FUNCTION__) << " command " << m_uiDataPins << " write when still BUSY" << '\n';
		return 0;
	}
	uint32_t delay = 0; // uS
	int four = !GetFlag(HD44780_FLAG_D_L);
	int comp = four && GetFlag(HD44780_FLAG_LOWNIBBLE);
	int write = 0;

	if (four) { // 4 bits !
		if (comp)
		{
			m_uiDataPins = (m_uiDataPins & 0xf0U) | (gsl::narrow<unsigned>(m_uiPinState >> gsl::narrow<uint8_t>(D4)) & 0xfU);
		}
		else
		{
			m_uiDataPins = (m_uiDataPins & 0xfU) | (gsl::narrow<unsigned>(m_uiPinState >> gsl::narrow<uint8_t>(D4-4U)) & 0xf0U);
		}
		write = comp;
		ToggleFlag(HD44780_FLAG_LOWNIBBLE);
	}
	else
	{	// 8 bits
		m_uiDataPins = gsl::narrow<uint8_t>(m_uiPinState >> gsl::narrow<uint8_t>(D0));
		write++;
	}
	RaiseIRQ(DATA_IN, m_uiDataPins);

	// write has 8 bits to process
	if (write)
	{
		if (m_uiPinState & (1U << RS))	// write data
		{
			delay = OnDataReady();
		}
		else
		{										// write command
			delay = OnCmdReady();
		}
	}
	return delay;
}

uint32_t HD44780::ProcessRead()
{
	uint32_t delay = 0; // uS
	int four = !GetFlag(HD44780_FLAG_D_L);
	int comp = four && GetFlag(HD44780_FLAG_LOWNIBBLE);
	int done = 0;	// has something on the datapin we want

	if (comp) {
		// ready the 4 final bits on the 'actual' lcd pins
		m_uiReadPins <<= 4;
		done++;
		ToggleFlag(HD44780_FLAG_LOWNIBBLE);
	}

	if (!done) { // new read

		if (m_uiPinState & (1U << RS)) {	// read data
			delay = 37;
			{
				std::lock_guard<std::mutex> lock(m_lock);
				m_uiReadPins = m_vRam[m_uiCursor];
			}
			IncrementCursor();
		} else {	// read 'command' ie status register
			delay = 0;	// no raising busy when reading busy !

			// low bits are the current cursor
			m_uiReadPins = m_uiCursor < 80 ? m_uiCursor : m_uiCursor-64;
			int busy = GetFlag(HD44780_FLAG_BUSY);
			m_uiReadPins |= busy ? 0x80 : 0;

		//	if (busy) printf("Good boy, guy's reading status byte\n");
			// now that we're read the busy flag, clear it and clear
			// the timer too
			SetFlag(HD44780_FLAG_BUSY, 0);
			RaiseIRQ(BUSY, 0);
			CancelTimer(m_fcnBusy,this);
		}
		RaiseIRQ(DATA_OUT, m_uiReadPins);

		done++;
		if (four)
		{
			SetFlag(HD44780_FLAG_LOWNIBBLE,1); // for next read
		}
	}

	// now send the prepared output pins to send as IRQs
	if (done)
	{
		RaiseIRQ(ALL, m_uiReadPins >> 4U);
		for (unsigned int i = four ? 4 : 0; i < 8; i++)
		{
			RaiseIRQ(D0 + i, static_cast<unsigned int>(m_uiReadPins >> i) & 1U);
		}
	}
	return delay;
}

avr_cycle_count_t HD44780::OnEPinChanged(struct avr_t *, avr_cycle_count_t)
{
    SetFlag(HD44780_FLAG_REENTRANT, 1);
	int delay = 0; // in uS

	if (m_uiPinState & (1U << RW))	// read !?!
	{
		delay = ProcessRead();
	}
	else
	{										// write
		delay = ProcessWrite();
	}

	if (delay) {
		SetFlag(HD44780_FLAG_BUSY, 1);
		RaiseIRQ(BUSY, 1);
		RegisterTimerUsec(m_fcnBusy, delay,this);
	}
	SetFlag(HD44780_FLAG_REENTRANT, 0);
	return 0;
}

void HD44780::OnPinChanged(struct avr_irq_t * irq,uint32_t value)
{
	uint16_t old = m_uiPinState;

	switch (irq->irq) {
		/*
		 * Update all the pins in one go by calling ourselves
		 * This is a shortcut for firmware that respects the conventions
		 */
		case ALL:
			for (unsigned int i = 0; i < 4; i++)
			{
				OnPinChanged(GetIRQ(D4+i),
						((value >> i) & 1U));
			}
			OnPinChanged(GetIRQ(RS), (value >> 4U));
			OnPinChanged(GetIRQ(E), (value >> 5U));
			OnPinChanged(GetIRQ(RW), (value >> 6U));
			return; // job already done!
		case D0 ... D7:
			// don't update these pins in read mode
			if (GetFlag(HD44780_FLAG_REENTRANT))
			{
				return;
			}
			break;
	}
	m_uiPinState = (m_uiPinState & ~(1U << irq->irq)) | (value << irq->irq);
	int eo = old & (1U << E);
	int e = m_uiPinState & (1U << E);
	// on the E pin falling edge, do stuff otherwise just exit
	if (eo && !e)
	{
		RegisterTimer(m_fcnEPinChanged,1,this);
	}
}

void HD44780::Init(avr_t *avr)
{
    _Init(avr,this);
	/*
	 * Register callbacks on all our IRQs
	 */

	for (int i = 0; i < BUSY; i++)
	{
		RegisterNotify(ALL+i, MAKE_C_CALLBACK(HD44780,OnPinChanged), this);
	}

	ResetCursor();
    ClearScreen();

	// printf("LCD: %duS is %d cycles for your AVR\n",
	// 		37, (int)avr_usec_to_cycles(avr, 37));
	// printf("LCD: %duS is %d cycles for your AVR\n",
	// 		1, (int)avr_usec_to_cycles(avr, 1));

	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, E, {TC::Display, TC::OutputPin});
	TH.AddTrace(this, RS, {TC::Display, TC::OutputPin});
	TH.AddTrace(this, RW, {TC::Display, TC::OutputPin});
	TH.AddTrace(this, D4, {TC::Display});
	TH.AddTrace(this, D5, {TC::Display});
	TH.AddTrace(this, D6, {TC::Display});
	TH.AddTrace(this, D7, {TC::Display});
	TH.AddTrace(this, DATA_IN, {TC::Display},8);
	TH.AddTrace(this, BRIGHTNESS_IN, {TC::Display, TC::OutputPin});
	TH.AddTrace(this, BRIGHTNESS_PWM_IN, {TC::Display, TC::PWM});
}
