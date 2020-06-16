/* HD44780.cpp

	Original based on simavr hd44780.c (C) 2011 Michel Pollet <buserror@gmail.com>

    Rewritten for C++ in 2020, VintagePC <https://github.com/vintagepc/>

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sim_time.h>
#include <avr_timer.h>
#include "HD44780.h"

//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif

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
    memset(m_vRam, ' ', sizeof(m_vRam));
	SetFlag(HD44780_FLAG_DIRTY, 1);
	RaiseIRQ(ADDR, m_uiCursor);
}

/*
 * This is called when the delay between operation is triggered
 * without the AVR firmware 'reading' the status byte. It
 * automatically clears the BUSY flag for the next command
 */
avr_cycle_count_t HD44780::OnBusyTimeout(struct avr_t * avr,avr_cycle_count_t when)
{
	SetFlag(HD44780_FLAG_BUSY, 0);
	RaiseIRQ(BUSY, 0);
	return 0;
}

Scriptable::LineStatus HD44780::ProcessAction(unsigned int iAction, const vector<string> &vArgs)
{
	switch (iAction)
	{
		case ActDesync:
			ToggleFlag(HD44780_FLAG_LOWNIBBLE);
			return LineStatus::Finished;
		case ActWaitForText:
			if (!m_uiLineChg) // NO changes to check against.
			return LineStatus::Waiting;

			int iLine = stoi(vArgs.at(1));
			if (iLine>=m_uiHeight || iLine<-1)
			{
				printf("LCD: Line index %d out of range. Valid: 0-%d and -1.\n",iLine, m_uiHeight);
				return LineStatus::Error;
			}
			bool bResult = false;
			// It might be possible to improve this by searching only the changed line region, but that is
			// not straightforward with strstr and doing it with std::string requires a copy.
			// Maybe with c++17 string_view, which lets you take a non-copy reference to char[].
			char* pFind = strstr((char*)m_vRam,vArgs.at(0).c_str());
			if (iLine<0)
				bResult =  pFind != NULL;
			else
			{
				int iLoc = (pFind-(char*)m_vRam);
				bResult = iLoc >=m_lineOffsets[iLine] && iLoc < (m_lineOffsets[iLine] + m_uiWidth);
	}
			return bResult ? LineStatus::Finished : LineStatus::Waiting;
}
}

void HD44780::IncrementCursor()
{

	if (GetFlag(HD44780_FLAG_I_D)) {
		TRACE(printf("Cursor++ (%02x)\n",m_uiCursor));
		if (m_uiCursor == 0x67) // end of display.
			m_uiCursor = 0x00; // wrap.
		else if (m_uiCursor == 0x27)
			m_uiCursor = 0x40;
		else
			m_uiCursor++;
	}
	else
	 {
		TRACE(printf("Cursor--\n"));
		if (m_uiCursor == 0x00)
			m_uiCursor = 0x67;
		else if (m_uiCursor == 0x40)
			m_uiCursor = 0x27;
		else
			m_uiCursor--;

		//SetFlag(HD44780_FLAG_DIRTY, 1);
		//avr_raise_irq(b->irq + ADDR, m_uiCursor);
	}
}

// Nudge the CGRAM cursor value.
void HD44780::IncrementCGRAMCursor()
{
	if (GetFlag(HD44780_FLAG_I_D))
		if (m_uiCGCursor==64)
			m_uiCGCursor = 0;
		else
			m_uiCGCursor++;
	else
		if (m_uiCGCursor==0)
			m_uiCGCursor = 64;
		else
			m_uiCGCursor--;
}

/*
 * current data byte is ready in m_uiDataPins
 */
uint32_t HD44780::OnDataReady()
{
	uint32_t delay = 37; // uS
	if (m_bInCGRAM)
	{
		m_cgRam[m_uiCGCursor] = m_uiDataPins;
		TRACE(printf("hd44780_write_data %02x to CGRAM %02x\n",m_uiDataPins,m_uiCGCursor));
		IncrementCGRAMCursor();
	}
	else
	{
		m_vRam[m_uiCursor] = m_uiDataPins;

		for (int i=0; i<m_uiHeight; i++) // Flag line change for search performance.
			if (m_uiCursor>= m_lineOffsets[i] && m_uiCursor< (m_lineOffsets[i] + m_uiWidth))
				m_uiLineChg |= 1<<i;

		TRACE(printf("hd44780_write_data %02x (%c) to %02x\n", m_uiDataPins, m_uiDataPins, m_uiCursor));
		if (GetFlag(HD44780_FLAG_S_C)) {	// display shift ?
			printf("Display shift requested. Not implemented, sorry!\n");
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
	int top = 7;	// get highest bit set'm
	while (top)
		if (m_uiDataPins & (1 << top))
			break;
		else top--;
	TRACE(printf("hd44780_write_command %02x (top: %u)\n", m_uiDataPins,top));
	switch (top) {
		// Set	DDRAM address
		case 7:		// 1 ADD ADD ADD ADD ADD ADD ADD
			m_uiCursor = m_uiDataPins & 0x7f;
			m_bInCGRAM = 0;
			break;
		// Set	CGRAM address
		case 6:		// 0 1 ADD ADD ADD ADD ADD ADD ADD
			TRACE(printf("cgram enter\n"));
			m_bInCGRAM = 1;
			m_uiCGCursor = (m_uiDataPins & 0x3f);
			break;
		// Function	set
		case 5:		// 0 0 1 DL N F x x
		{
			int four = !GetFlag(HD44780_FLAG_D_L);
			SetFlag(HD44780_FLAG_D_L, m_uiDataPins & 16);
			SetFlag(HD44780_FLAG_N, m_uiDataPins & 8);
			SetFlag(HD44780_FLAG_F, m_uiDataPins & 4);
			if (!four && !GetFlag(HD44780_FLAG_D_L)) {
				printf("%s activating 4 bits mode\n", __FUNCTION__);
				SetFlag(HD44780_FLAG_LOWNIBBLE, 0);
			}
		}
			break;
		// Cursor display shift
		case 4:		// 0 0 0 1 S/C R/L x x
			SetFlag(HD44780_FLAG_S_C, m_uiDataPins & 8);
			SetFlag(HD44780_FLAG_R_L, m_uiDataPins & 4);
			break;
		// Display on/off control
		case 3:		// 0 0 0 0 1 D C B
			SetFlag(HD44780_FLAG_D, m_uiDataPins & 4);
			SetFlag(HD44780_FLAG_C, m_uiDataPins & 2);
			SetFlag(HD44780_FLAG_B, m_uiDataPins & 1);
			SetFlag(HD44780_FLAG_DIRTY, 1);
			break;
		// Entry mode set
		case 2:		// 0 0 0 0 0 1 I/D S
			SetFlag(HD44780_FLAG_I_D, m_uiDataPins & 2);
			SetFlag(HD44780_FLAG_S, m_uiDataPins & 1);
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
	uint32_t delay = 0; // uS
	int four = !GetFlag(HD44780_FLAG_D_L);
	int comp = four && GetFlag(HD44780_FLAG_LOWNIBBLE);
	int write = 0;

	if (four) { // 4 bits !
		if (comp)
			m_uiDataPins = (m_uiDataPins & 0xf0) | ((m_uiPinState >>  D4) & 0xf);
		else
			m_uiDataPins = (m_uiDataPins & 0xf) | ((m_uiPinState >>  (D4-4)) & 0xf0);
		write = comp;
		ToggleFlag(HD44780_FLAG_LOWNIBBLE);
	} else {	// 8 bits
		m_uiDataPins = (m_uiPinState >>  D0) & 0xff;
		write++;
	}
	RaiseIRQ(DATA_IN, m_uiDataPins);

	// write has 8 bits to process
	if (write) {
		if (GetFlag(HD44780_FLAG_BUSY)) {
			printf("%s command %02x write when still BUSY\n", __FUNCTION__, m_uiDataPins);
		}
		if (m_uiPinState & (1 << RS))	// write data
			delay = OnDataReady();
		else										// write command
			delay = OnCmdReady();
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

		if (m_uiPinState & (1 << RS)) {	// read data
			delay = 37;
			m_uiReadPins = m_vRam[m_uiCursor];
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
			SetFlag(HD44780_FLAG_LOWNIBBLE,1); // for next read
	}

	// now send the prepared output pins to send as IRQs
	if (done) {
		RaiseIRQ(ALL, m_uiReadPins >> 4);
		for (int i = four ? 4 : 0; i < 8; i++)
			RaiseIRQ(D0 + i, (m_uiReadPins >> i) & 1);
	}
	return delay;
}

avr_cycle_count_t HD44780::OnEPinChanged(struct avr_t * avr, avr_cycle_count_t when)
{
    SetFlag(HD44780_FLAG_REENTRANT, 1);

	int delay = 0; // in uS

	if (m_uiPinState & (1 << RW))	// read !?!
		delay = ProcessRead();
	else										// write
		delay = ProcessWrite();

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
			for (int i = 0; i < 4; i++)
				OnPinChanged(GetIRQ(D4) + i,
						((value >> i) & 1));
			OnPinChanged(GetIRQ(RS), (value >> 4));
			OnPinChanged(GetIRQ(E), (value >> 5));
			OnPinChanged(GetIRQ(RW), (value >> 6));
			return; // job already done!
		case D0 ... D7:
			// don't update these pins in read mode
			if (GetFlag(HD44780_FLAG_REENTRANT))
				return;
			break;
	}
	m_uiPinState = (m_uiPinState & ~(1 << irq->irq)) | (value << irq->irq);
	int eo = old & (1 << E);
	int e = m_uiPinState & (1 << E);
	// on the E pin rising edge, do stuff otherwise just exit
	if (!eo && e)
		RegisterTimer(m_fcnEPinChanged,1,this);
}

void HD44780::Init(avr_t *avr)
{
    _Init(avr,this);
	memset(m_cgRam, 0, sizeof(m_cgRam));
	memset(m_vRam, 0, sizeof(m_vRam));
	/*
	 * Register callbacks on all our IRQs
	 */

	for (int i = 0; i < BUSY; i++)
		RegisterNotify(ALL+i, MAKE_C_CALLBACK(HD44780,OnPinChanged), this);

	ResetCursor();
    ClearScreen();

	printf("LCD: %duS is %d cycles for your AVR\n",
			37, (int)avr_usec_to_cycles(avr, 37));
	printf("LCD: %duS is %d cycles for your AVR\n",
			1, (int)avr_usec_to_cycles(avr, 1));
}
