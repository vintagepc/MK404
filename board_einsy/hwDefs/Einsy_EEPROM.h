/*
	EEPROM helper to persist the AVR eeprom (and let us poke about in its internals...)
 */

#ifndef __EINSY_EEPROM_H__
#define __EINSY_EEPROM_H__

#include "stdlib.h"
#include "unistd.h"
#include "sim_avr.h"

// Loads EEPROM from a file
int einsy_eeprom_load(
		struct avr_t * avr,
		const char* path
        );

// Saves EEPROM to a file
void einsy_eeprom_save(
		struct avr_t * avr,
		const char* path, 
        int fd);

// Pokes something into the EEPROM.
void einsy_eeprom_poke(
		struct avr_t *avr,
		uint16_t address,
		uint8_t value
	);

// Peeks at an eeprom location.
uint8_t einsy_eeprom_peek(
		struct avr_t *avr,
		uint16_t address
	);
#endif
