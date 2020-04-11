/*
	hd44780.c

	Copyright Luki <humbell@ethz.ch>
	Copyright 2011 Michel Pollet <buserror@gmail.com>

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
#include "sim_time.h"
#include "avr_timer.h"
#include "hd44780.h"

//#define TRACE(_w) _w
#ifndef TRACE
#define TRACE(_w)
#endif

void
hd44780_print(
		hd44780_t *b)
{
	printf("/******************\\\n");
	const uint8_t offset[] =  {0, 0x40, 0x20, 0x60 };
	for (int i = 0; i < b->h; i++) {
		printf("| ");
		fwrite(b->vram + offset[i], 1, b->w, stdout);
		printf(" |\n");
	}
	printf("\\******************/\n");
}


static void
_hd44780_reset_cursor(
		hd44780_t *b)
{
	b->cursor = 0;
	b->bInCGRAM = 0;
	TRACE(printf("cursor reset\n"));
	hd44780_set_flag(b, HD44780_FLAG_DIRTY, 1);
	avr_raise_irq(b->irq + IRQ_HD44780_ADDR, b->cursor);
}

static void
_hd44780_clear_screen(
		hd44780_t *b)
{
	memset(b->vram, ' ', 0x67);
	hd44780_set_flag(b, HD44780_FLAG_DIRTY, 1);
	avr_raise_irq(b->irq + IRQ_HD44780_ADDR, b->cursor);
}



/*
 * This is called when the delay between operation is triggered
 * without the AVR firmware 'reading' the status byte. It
 * automatically clears the BUSY flag for the next command
 */
static avr_cycle_count_t
_hd44780_busy_timer(
		struct avr_t * avr,
        avr_cycle_count_t when, void * param)
{
	hd44780_t *b = (hd44780_t *) param;
//	printf("%s called\n", __FUNCTION__);
	hd44780_set_flag(b, HD44780_FLAG_BUSY, 0);
	avr_raise_irq(b->irq + IRQ_HD44780_BUSY, 0);
	return 0;
}

static void
hd44780_kick_cursor(
	hd44780_t *b)
{
	
	if (hd44780_get_flag(b, HD44780_FLAG_I_D)) {
		TRACE(printf("Cursor++ (%02x)\n",b->cursor));
		if (b->cursor == 0x67) // end of display.
			b->cursor = 0x00; // wrap.
		else if (b->cursor == 0x27)
			b->cursor = 0x40;
		else
			b->cursor++;
	} 
	else
	 {
		printf("Cursor--\n");
		if (b->cursor == 0x00)
			b->cursor = 0x67;
		else if (b->cursor == 0x40)
			b->cursor = 0x27;
		else
			b->cursor--;	
			
		//hd44780_set_flag(b, HD44780_FLAG_DIRTY, 1);
		//avr_raise_irq(b->irq + IRQ_HD44780_ADDR, b->cursor);
	}
}

// Nudge the CGRAM cursor value. 
static void 
hd44780_cgram_kick_cursor(
	hd44780_t *b)
{
	if (hd44780_get_flag(b, HD44780_FLAG_I_D)) 
		if (b->cg_cursor==64)
			b->cg_cursor = 0;
		else
			b->cg_cursor++;
	else
		if (b->cg_cursor==0)
			b->cg_cursor = 64;
		else
			b->cg_cursor--;
}

// Dumps the current CGRAM char to console
static void hd44780_dump_cg_char(hd44780_t *b)
{
	uint8_t uiBase = b->cg_cursor & 0b111000;
	uint8_t uiChar = 0;
	for (int i=0; i<8; i++)
	{
	 uiChar = b->cgram[uiBase+i];
		printf("%u%u%u%u%u\n", 
			(uiChar & 1)==1,
			(uiChar & 2)>1,
			(uiChar & 4)>1,
			(uiChar & 8)>1,
			(uiChar & 16)>1);
	}
}

/*
 * current data byte is ready in b->datapins
 */
static uint32_t
hd44780_write_data(
		hd44780_t *b)
{
	uint32_t delay = 37; // uS
	if (b->bInCGRAM)
	{
		b->cgram[b->cg_cursor] = b->datapins;
		TRACE(printf("hd44780_write_data %02x to CGRAM %02x\n",b->datapins,b->cg_cursor));		
		if ((b->cg_cursor & 7) == 7) // Crossed a char boundary
		{
			TRACE(hd44780_dump_cg_char(b));
		}
		hd44780_cgram_kick_cursor(b);
		hd44780_set_flag(b,HD44780_FLAG_CRAM_DIRTY,1);
		
	}
	else
	{
		b->vram[b->cursor] = b->datapins;
		TRACE(printf("hd44780_write_data %02x (%c) to %02x\n", b->datapins, b->datapins, b->cursor));
		if (hd44780_get_flag(b, HD44780_FLAG_S_C)) {	// display shift ?
			// TODO display shift
		} else {
			hd44780_kick_cursor(b);
		}
		hd44780_set_flag(b, HD44780_FLAG_DIRTY, 1);
	}
	return delay;
}

/*
 * current command is ready in b->datapins
 */
static uint32_t
hd44780_write_command(
		hd44780_t *b)
{
	uint32_t delay = 37; // uS
	int top = 7;	// get highest bit set'm
	while (top)
		if (b->datapins & (1 << top))
			break;
		else top--;
	TRACE(printf("hd44780_write_command %02x (top: %u)\n", b->datapins,top));
	switch (top) {
		// Set	DDRAM address
		case 7:		// 1 ADD ADD ADD ADD ADD ADD ADD
			b->cursor = b->datapins & 0x7f;
			b->bInCGRAM = 0;
			break;
		// Set	CGRAM address
		case 6:		// 0 1 ADD ADD ADD ADD ADD ADD ADD
			TRACE(printf("cgram enter\n"));
			b->bInCGRAM = 1;
			b->cg_cursor = (b->datapins & 0x3f);
			break;
		// Function	set
		case 5:		// 0 0 1 DL N F x x
		{
			int four = !hd44780_get_flag(b, HD44780_FLAG_D_L);
			hd44780_set_flag(b, HD44780_FLAG_D_L, b->datapins & 16);
			hd44780_set_flag(b, HD44780_FLAG_N, b->datapins & 8);
			hd44780_set_flag(b, HD44780_FLAG_F, b->datapins & 4);
			if (!four && !hd44780_get_flag(b, HD44780_FLAG_D_L)) {
				printf("%s activating 4 bits mode\n", __FUNCTION__);
				hd44780_set_flag(b, HD44780_FLAG_LOWNIBBLE, 0);
			}
		}
			break;
		// Cursor display shift
		case 4:		// 0 0 0 1 S/C R/L x x
			hd44780_set_flag(b, HD44780_FLAG_S_C, b->datapins & 8);
			hd44780_set_flag(b, HD44780_FLAG_R_L, b->datapins & 4);
			break;
		// Display on/off control
		case 3:		// 0 0 0 0 1 D C B
			hd44780_set_flag(b, HD44780_FLAG_D, b->datapins & 4);
			hd44780_set_flag(b, HD44780_FLAG_C, b->datapins & 2);
			hd44780_set_flag(b, HD44780_FLAG_B, b->datapins & 1);
			hd44780_set_flag(b, HD44780_FLAG_DIRTY, 1);
			break;
		// Entry mode set
		case 2:		// 0 0 0 0 0 1 I/D S
			hd44780_set_flag(b, HD44780_FLAG_I_D, b->datapins & 2);
			hd44780_set_flag(b, HD44780_FLAG_S, b->datapins & 1);
			break;
		// Return home
		case 1:		// 0 0 0 0 0 0 1 x
			_hd44780_reset_cursor(b);
			delay = 1520;
			break;
		// Clear display
		case 0:		// 0 0 0 0 0 0 0 1
			_hd44780_clear_screen(b);
			_hd44780_reset_cursor(b);
			break;
	}
	return delay;
}

/*
 * the E pin went low, and it's a write
 */
static uint32_t
hd44780_process_write(
		hd44780_t *b )
{
	uint32_t delay = 0; // uS
	int four = !hd44780_get_flag(b, HD44780_FLAG_D_L);
	int comp = four && hd44780_get_flag(b, HD44780_FLAG_LOWNIBBLE);
	int write = 0;

	if (four) { // 4 bits !
		if (comp)
			b->datapins = (b->datapins & 0xf0) | ((b->pinstate >>  IRQ_HD44780_D4) & 0xf);
		else
			b->datapins = (b->datapins & 0xf) | ((b->pinstate >>  (IRQ_HD44780_D4-4)) & 0xf0);
		write = comp;
		b->flags ^= (1 << HD44780_FLAG_LOWNIBBLE);
	} else {	// 8 bits
		b->datapins = (b->pinstate >>  IRQ_HD44780_D0) & 0xff;
		write++;
	}
	avr_raise_irq(b->irq + IRQ_HD44780_DATA_IN, b->datapins);

	// write has 8 bits to process
	if (write) {
		if (hd44780_get_flag(b, HD44780_FLAG_BUSY)) {
			printf("%s command %02x write when still BUSY\n", __FUNCTION__, b->datapins);
		}
		if (b->pinstate & (1 << IRQ_HD44780_RS))	// write data
			delay = hd44780_write_data(b);
		else										// write command
			delay = hd44780_write_command(b);
	}
	return delay;
}

static uint32_t
hd44780_process_read(
		hd44780_t *b )
{
	uint32_t delay = 0; // uS
	int four = !hd44780_get_flag(b, HD44780_FLAG_D_L);
	int comp = four && hd44780_get_flag(b, HD44780_FLAG_LOWNIBBLE);
	int done = 0;	// has something on the datapin we want

	if (comp) {
		// ready the 4 final bits on the 'actual' lcd pins
		b->readpins <<= 4;
		done++;
		b->flags ^= (1 << HD44780_FLAG_LOWNIBBLE);
	}

	if (!done) { // new read

		if (b->pinstate & (1 << IRQ_HD44780_RS)) {	// read data
			delay = 37;
			b->readpins = b->vram[b->cursor];
			hd44780_kick_cursor(b);
		} else {	// read 'command' ie status register
			delay = 0;	// no raising busy when reading busy !

			// low bits are the current cursor
			b->readpins = b->cursor < 80 ? b->cursor : b->cursor-64;
			int busy = hd44780_get_flag(b, HD44780_FLAG_BUSY);
			b->readpins |= busy ? 0x80 : 0;

		//	if (busy) printf("Good boy, guy's reading status byte\n");
			// now that we're read the busy flag, clear it and clear
			// the timer too
			hd44780_set_flag(b, HD44780_FLAG_BUSY, 0);
			avr_raise_irq(b->irq + IRQ_HD44780_BUSY, 0);
			avr_cycle_timer_cancel(b->avr, _hd44780_busy_timer, b);
		}
		avr_raise_irq(b->irq + IRQ_HD44780_DATA_OUT, b->readpins);

		done++;
		if (four)
			b->flags |= (1 << HD44780_FLAG_LOWNIBBLE); // for next read
	}

	// now send the prepared output pins to send as IRQs
	if (done) {
		avr_raise_irq(b->irq + IRQ_HD44780_ALL, b->readpins >> 4);
		for (int i = four ? 4 : 0; i < 8; i++)
			avr_raise_irq(b->irq + IRQ_HD44780_D0 + i, (b->readpins >> i) & 1);
	}
	return delay;
}

static avr_cycle_count_t
_hd44780_process_e_pinchange(
		struct avr_t * avr,
        avr_cycle_count_t when, void * param)
{
	hd44780_t *b = (hd44780_t *) param;

	hd44780_set_flag(b, HD44780_FLAG_REENTRANT, 1);

#if 0
	uint16_t touch = b->oldstate ^ b->pinstate;
	printf("LCD: %04x %04x %c %c %c %c\n", b->pinstate, touch,
			b->pinstate & (1 << IRQ_HD44780_RW) ? 'R' : 'W',
			b->pinstate & (1 << IRQ_HD44780_RS) ? 'D' : 'C',
			hd44780_get_flag(b, HD44780_FLAG_LOWNIBBLE) ? 'L' : 'H',
			hd44780_get_flag(b, HD44780_FLAG_BUSY) ? 'B' : ' ');
#endif
	int delay = 0; // in uS

	if (b->pinstate & (1 << IRQ_HD44780_RW))	// read !?!
		delay = hd44780_process_read(b);
	else										// write
		delay = hd44780_process_write(b);

	if (delay) {
		hd44780_set_flag(b, HD44780_FLAG_BUSY, 1);
		avr_raise_irq(b->irq + IRQ_HD44780_BUSY, 1);
		avr_cycle_timer_register_usec(b->avr, delay,
			_hd44780_busy_timer, b);
	}
//	b->oldstate = b->pinstate;
	hd44780_set_flag(b, HD44780_FLAG_REENTRANT, 0);
	return 0;
}

static void
hd44780_brightness_changed_hook(
		struct avr_irq_t * irq,
		uint32_t value,
        void *param)
{
	hd44780_t *b = (hd44780_t *) param;
	TRACE(printf("Brightness pin changed value: %u\n",value));
	b->iBrightness = (uint8_t)value;
	avr_raise_irq(b->irq + IRQ_HD44780_BRIGHTNESS_OUT,0); // Force low to enable detect.
	hd44780_set_flag(b,HD44780_FLAG_DIRTY,1);
}

static void
hd44780_pin_changed_hook(
		struct avr_irq_t * irq,
		uint32_t value,
        void *param)
{
	hd44780_t *b = (hd44780_t *) param;

	uint16_t old = b->pinstate;

	switch (irq->irq) {
		/*
		 * Update all the pins in one go by calling ourselves
		 * This is a shortcut for firmware that respects the conventions
		 */
		case IRQ_HD44780_ALL:
			for (int i = 0; i < 4; i++)
				hd44780_pin_changed_hook(b->irq + IRQ_HD44780_D4 + i,
						((value >> i) & 1), param);
			hd44780_pin_changed_hook(b->irq + IRQ_HD44780_RS, (value >> 4), param);
			hd44780_pin_changed_hook(b->irq + IRQ_HD44780_E, (value >> 5), param);
			hd44780_pin_changed_hook(b->irq + IRQ_HD44780_RW, (value >> 6), param);
			return; // job already done!
		case IRQ_HD44780_D0 ... IRQ_HD44780_D7:
			// don't update these pins in read mode
			if (hd44780_get_flag(b, HD44780_FLAG_REENTRANT))
				return;
			break;
	}
	b->pinstate = (b->pinstate & ~(1 << irq->irq)) | (value << irq->irq);
	int eo = old & (1 << IRQ_HD44780_E);
	int e = b->pinstate & (1 << IRQ_HD44780_E);
	// on the E pin rising edge, do stuff otherwise just exit
	if (!eo && e)
		avr_cycle_timer_register(b->avr, 1, _hd44780_process_e_pinchange, b);
}

static const char * irq_names[IRQ_HD44780_COUNT] = {
	[IRQ_HD44780_ALL] = "7=hd44780.pins",
	[IRQ_HD44780_RS] = "<hd44780.RS",
	[IRQ_HD44780_RW] = "<hd44780.RW",
	[IRQ_HD44780_E] = "<hd44780.E",
	[IRQ_HD44780_D0] = "=hd44780.D0",
	[IRQ_HD44780_D1] = "=hd44780.D1",
	[IRQ_HD44780_D2] = "=hd44780.D2",
	[IRQ_HD44780_D3] = "=hd44780.D3",
	[IRQ_HD44780_D4] = "=hd44780.D4",
	[IRQ_HD44780_D5] = "=hd44780.D5",
	[IRQ_HD44780_D6] = "=hd44780.D6",
	[IRQ_HD44780_D7] = "=hd44780.D7",

	[IRQ_HD44780_BUSY] = ">hd44780.BUSY",
	[IRQ_HD44780_ADDR] = "7>hd44780.ADDR",
	[IRQ_HD44780_DATA_IN] = "8>hd44780.DATA_IN",
	[IRQ_HD44780_DATA_OUT] = "8>hd44780.DATA_OUT",
	[IRQ_HD44780_BRIGHTNESS] = "=hd44780.BRIGHTNESS"
};

void
hd44780_init(
		struct avr_t *avr,
		struct hd44780_t * b,
		int width,
		int height )
{
	memset(b, 0, sizeof(*b));
	b->avr = avr;
	b->w = width;
	b->h = height;
	b->iBrightness = 255;
	/*
	 * Register callbacks on all our IRQs
	 */
	b->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_HD44780_COUNT, irq_names);
	for (int i = 0; i < IRQ_HD44780_INPUT_COUNT; i++)
		avr_irq_register_notify(b->irq + i, hd44780_pin_changed_hook, b);

	// Attach to PWM control signal
	avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_TIMER_GETIRQ('3'),TIMER_IRQ_OUT_PWM0),hd44780_brightness_changed_hook,b);

	_hd44780_reset_cursor(b);
	_hd44780_clear_screen(b);

	printf("LCD: %duS is %d cycles for your AVR\n",
			37, (int)avr_usec_to_cycles(avr, 37));
	printf("LCD: %duS is %d cycles for your AVR\n",
			1, (int)avr_usec_to_cycles(avr, 1));
}

