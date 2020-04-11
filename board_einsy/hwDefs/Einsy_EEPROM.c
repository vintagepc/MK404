/*
	EEPROM helper to persist the AVR eeprom (and let us poke about in its internals...)
 */
#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include <fcntl.h>
#include "string.h"
#include "Einsy_EEPROM.h"
#include "sim_mega2560.h"

int einsy_eeprom_load(
		struct avr_t * avr,
		const char* path)
{
	// Now deal with the onboard EEPROM. Can't do this in special_init, it's not allocated yet then.
	int fd = open(path, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		perror(path);
		exit(1);
	}
	struct mcu_t * mcu = (struct mcu_t*)avr;
	printf("Loading %u bytes of EEPROM\n",mcu->eeprom.size);
	(void)ftruncate(fd, mcu->eeprom.size +1);
	uint8_t *buffer = malloc(mcu->eeprom.size+1);
	ssize_t r = read(fd, buffer, mcu->eeprom.size+1);
	printf("Read %d bytes\n",(int)r);
	if (r !=  mcu->eeprom.size + 1) {
		fprintf(stderr, "unable to load EEPROM\n");
		perror(path);
		exit(1);
	}
	uint8_t bEmpty = 0;
	for (int i=0; i<mcu->eeprom.size +1; i++)
	{
		bEmpty |= buffer[i]==0;
	}
	if (!bEmpty) // If the file was newly created (all null) this leaves the internal eeprom as full of 0xFFs.
		memcpy(mcu->eeprom.eeprom, buffer, mcu->eeprom.size +1);

    free(buffer);
    return fd;
}

void einsy_eeprom_save(
		struct avr_t * avr,
		const char* path, 
        int fd)
{
	// Also write out the EEPROM contents:
	lseek(fd, SEEK_SET, 0);
	struct mcu_t * mcu = (struct mcu_t*)avr;
	ssize_t r = write(fd, mcu->eeprom.eeprom, mcu->eeprom.size+1);
	if (r != mcu->eeprom.size+1) {
		fprintf(stderr, "unable to write EEPROM memory\n");
		perror(path);
	}
	close(fd);
}


void einsy_eeprom_poke(
		struct avr_t *avr,
		uint16_t address,
		uint8_t value
	)
    {
        struct mcu_t * mcu = (struct mcu_t*)avr;
        assert(address<=mcu->eeprom.size);
        mcu->eeprom.eeprom[address] = value;
    }

uint8_t einsy_eeprom_peek(
		struct avr_t *avr,
		uint16_t address
	)
    {
        struct mcu_t * mcu = (struct mcu_t*)avr;
        assert(address<=mcu->eeprom.size);
        return mcu->eeprom.eeprom[address];
    }

