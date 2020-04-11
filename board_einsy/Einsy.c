/*
	simduino.c

	Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

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

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>

#if __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <pthread.h>

#define TEMP_SENSOR_0 5
#define TEMP_SENSOR_BED 1
#define TEMP_SENSOR_AMBIENT 2000

#define _TERMISTOR_TABLE(num) \
		temptable_##num
#define TERMISTOR_TABLE(num) \
		_TERMISTOR_TABLE(num)

#include "sim_avr.h"
#include "avr_ioport.h"
#include "avr_spi.h"
#include "avr_eeprom.h"
#include "sim_elf.h"
#include "sim_hex.h"
#include "sim_gdb.h"
#include "uart_pty.h"
#include "hd44780_glut.h"
#include "rotenc.h"
#include "button.h"
#include "thermistor.h"
#include "thermistortables.h"
#include "sim_vcd_file.h"
#include "w25x20cl.h"
#include "Firmware/eeprom.h"
#include "Einsy_EEPROM.h"

avr_t * avr = NULL;
avr_vcd_t vcd_file;

uint8_t gbStop = 0;
uint8_t gbPrintPC = 0;

struct avr_flash {
	char avr_flash_path[1024];
	int avr_flash_fd;
	char avr_eeprom_path[1024];
	int avr_eeprom_fd;
};

int window;

uint32_t colors[4] = {
		0x02c5fbff, 0x8d7ff8ff, 0xFFFFFFff, 0x00000055 
};

struct hw_t {
	hd44780_t lcd;
	rotenc_t encoder;
	button_t powerPanic;
	uart_pty_t UART0, UART1, UART2, UART3;
	thermistor_t tExtruder, tBed, tPinda, tAmbient;
	w25x20cl_t spiFlash;
} hw;

unsigned char guKey = 0;

// avr special flash initalization
// here: open and map a file to enable a persistent storage for the flash memory
void avr_special_init( avr_t * avr, void * data)
{
	struct avr_flash *flash_data = (struct avr_flash *)data;

	printf("%s\n", __func__);
	// open the file
	flash_data->avr_flash_fd = open(flash_data->avr_flash_path,
									O_RDWR|O_CREAT, 0644);
	if (flash_data->avr_flash_fd < 0) {
		perror(flash_data->avr_flash_path);
		exit(1);
	}
	// resize and map the file the file
	(void)ftruncate(flash_data->avr_flash_fd, avr->flashend + 1);
	ssize_t r = read(flash_data->avr_flash_fd, avr->flash, avr->flashend + 1);
	if (r != avr->flashend + 1) {
		fprintf(stderr, "unable to load flash memory\n");
		perror(flash_data->avr_flash_path);
		exit(1);
	}
}

// avr special flash deinitalization
// here: cleanup the persistent storage
void avr_special_deinit( avr_t* avr, void * data)
{
	struct avr_flash *flash_data = (struct avr_flash *)data;

	printf("%s\n", __func__);
	lseek(flash_data->avr_flash_fd, SEEK_SET, 0);
	ssize_t r = write(flash_data->avr_flash_fd, avr->flash, avr->flashend + 1);
	if (r != avr->flashend + 1) {
		fprintf(stderr, "unable to write flash memory\n");
		perror(flash_data->avr_flash_path);
	}
	close(flash_data->avr_flash_fd);

	einsy_eeprom_save(avr, flash_data->avr_eeprom_path, flash_data->avr_eeprom_fd);

	uart_pty_stop(&hw.UART0);
	uart_pty_stop(&hw.UART1);
}

void displayCB(void)		/* function called whenever redisplay needed */
{
	if (hd44780_get_flag(&hw.lcd, HD44780_FLAG_DIRTY)==0 && 
		hd44780_get_flag(&hw.lcd, HD44780_FLAG_CRAM_DIRTY == 0))
		return;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW); // Select modelview matrix
	glPushMatrix();
	glLoadIdentity(); // Start with an identity matrix
	glScalef(4, 4, 1);

	hd44780_gl_draw(
		&hw.lcd,
			colors[0], /* background */
			colors[1], /* character background */
			colors[2], /* text */
			colors[3] /* shadow */ );
	glPopMatrix();
	glutSwapBuffers();
}


static void *
avr_run_thread(
		void * ignore)
{
	printf("Starting AVR execution...\n");
	int state = cpu_Running;
	while ((state != cpu_Done) && (state != cpu_Crashed)){
		
	
		// Interactive key handling:
		if (guKey) {
			switch (guKey) {
				case 'w':
					printf("CCW turn\n");
					rotenc_twist(&hw.encoder, ROTENC_CCW_CLICK);
					break;
				case 's':
					printf("CW turn\n");
					rotenc_twist(&hw.encoder, ROTENC_CW_CLICK);
					break;
				case 0xd:
					printf("ENTER pushed\n");
					rotenc_button_press(&hw.encoder);
					break;
				case 'r':
					printf("RESET/KILL\n");
					// RESET BUTTON
					avr_reset(avr);
					avr_raise_irq(hw.encoder.irq + IRQ_ROTENC_OUT_BUTTON_PIN, 0);
					break;
				case 't':
					printf("FACTORY_RESET\n");
					// Hold the button during boot to get factory reset menu
					avr_reset(avr);
					rotenc_button_press_hold(&hw.encoder);
					break;
				case 'f':
					printf("Forcing display redraw\n");
					hd44780_set_flag(&hw.lcd, HD44780_FLAG_DIRTY,1) ;
					displayCB();
					break;
				case 'q':
					gbStop = 1;
					break;
			}
			if (gbStop)
			{
				break;
			}
			guKey = 0;
		}
		if (gbPrintPC)
			printf("PC: %x\n",avr->pc);
		state = avr_run(avr);
		
	}

	printf("Writing flash state...\n");
	avr_terminate(avr);
	printf("AVR finished.\n");
	return NULL;
}

void keyCB(
		unsigned char key, int x, int y)	/* called on key press */
{
	printf("Keypress: %x\n",key);
	switch (key) {
		case 'q':
			//glutLeaveMainLoop();
			guKey = key;
			break;
		case 'p':
			printf("SIMULATING POWER PANIC\n");
			avr_raise_irq(hw.powerPanic.irq + IRQ_BUTTON_OUT, 1);
			break;
		case 'd':
			gbPrintPC = gbPrintPC==0;
			break;
		/* case 'r':
			printf("Starting VCD trace; press 's' to stop\n");
			avr_vcd_start(&vcd_file);
			break;
		case 's':
			printf("Stopping VCD trace\n");
			avr_vcd_stop(&vcd_file);
			break */;
		default:
			guKey = key;
	}
}


// gl timer. if the lcd is dirty, refresh display
void timerCB(int i)
{
	//static int oldstate = -1;
	// restart timer
	glutTimerFunc(1000, timerCB, 0);
	glutPostRedisplay();
	//hd44780_print(&hd44780);
}

int initGL(int w, int h)
{
	// Set up projection matrix
	glMatrixMode(GL_PROJECTION); // Select projection matrix
	glLoadIdentity(); // Start with an identity matrix
	glOrtho(0, w, 0, h, 0, 10);
	glScalef(1,-1,1);
	glTranslatef(0, -1 * h, 0);

	glutDisplayFunc(displayCB);		/* set window's display callback */
	glutKeyboardFunc(keyCB);		/* set window's key callback */
	glutTimerFunc(1000, timerCB, 0);

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	hd44780_gl_init();

	return 1;
}

void setupLCD()
{
	hd44780_init(avr, &hw.lcd, 20,4);
	hd44780_set_flag(&hw.lcd, HD44780_FLAG_LOWNIBBLE, 0);
	// D4-D7,
	int iPin[4] = {5,4,7,3};
	int iIRQ[5] = {AVR_IOCTL_IOPORT_GETIRQ('F'),AVR_IOCTL_IOPORT_GETIRQ('G'),
		AVR_IOCTL_IOPORT_GETIRQ('H'),AVR_IOCTL_IOPORT_GETIRQ('G'),
		AVR_IOCTL_IOPORT_GETIRQ('D')};
	for (int i = 0; i < 4; i++) {
		avr_irq_t * iavr = avr_io_getirq(avr, iIRQ[i], iPin[i]);
		avr_irq_t * ilcd = hw.lcd.irq + IRQ_HD44780_D4 + i;
		// AVR -> LCD
		avr_connect_irq(iavr, ilcd);
		// LCD -> AVR
		avr_connect_irq(ilcd, iavr);
	}
	avr_connect_irq(
		avr_io_getirq(avr, iIRQ[4], 5),
		hw.lcd.irq + IRQ_HD44780_RS);
	avr_connect_irq(
		avr_io_getirq(avr, iIRQ[0], 7),
		hw.lcd.irq + IRQ_HD44780_E);

	avr_connect_irq(avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('E'),3),
			hw.lcd.irq + IRQ_HD44780_BRIGHTNESS);

	rotenc_init(avr, &hw.encoder);

	avr_connect_irq(hw.encoder.irq + IRQ_ROTENC_OUT_A_PIN,
	avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('J'),1));

	avr_connect_irq(hw.encoder.irq + IRQ_ROTENC_OUT_B_PIN,
	avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('J'),2));

	avr_connect_irq(hw.encoder.irq + IRQ_ROTENC_OUT_BUTTON_PIN,
	avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('H'),6));
}

void setupSerial()
{
	uart_pty_init(avr, &hw.UART0);
	uart_pty_init(avr, &hw.UART1);
	uart_pty_init(avr, &hw.UART2);
	uart_pty_init(avr, &hw.UART3);

	w25x20cl_init(avr, &hw.spiFlash);

	// Wire up the SPI
	avr_connect_irq(avr_io_getirq(avr, AVR_IOCTL_SPI_GETIRQ(0),0), 
		hw.spiFlash.irq + IRQ_W25X20CL_SPI_BYTE_IN);

	//uart_pty_connect(&hw.UART0, '0');

	// Setup UART1. 

	//uart_pty_connect(&hwUART1,'1');
}

void setupHeaters()
{
	thermistor_init(avr, &hw.tExtruder, 0,
		(short*)TERMISTOR_TABLE(TEMP_SENSOR_0),
		sizeof(TERMISTOR_TABLE(TEMP_SENSOR_0)) / sizeof(short) / 2,
		OVERSAMPLENR, 183.0f);

		 thermistor_init(avr, &hw.tBed, 2,
		 (short*)TERMISTOR_TABLE(TEMP_SENSOR_BED),
		 sizeof(TERMISTOR_TABLE(TEMP_SENSOR_BED)) / sizeof(short) / 2,
		 OVERSAMPLENR, 63.0f);

		// same table as bed.
		thermistor_init(avr, &hw.tPinda, 3,
		 (short*)TERMISTOR_TABLE(TEMP_SENSOR_BED),
		 sizeof(TERMISTOR_TABLE(TEMP_SENSOR_BED)) / sizeof(short) / 2,
		 OVERSAMPLENR, 63.0f);

		thermistor_init(avr, &hw.tAmbient, 6,
		 (short*)TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT),
		 sizeof(TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT)) / sizeof(short) / 2,
		 OVERSAMPLENR, 35.0f);
}

int main(int argc, char *argv[])
{
	struct avr_flash flash_data;
	char boot_path[1024] = "stk500boot_v2_mega2560.hex";
	//char boot_path[1024] = "atmega2560_PFW.axf";
	uint32_t boot_base, boot_size;
	char * mmcu = "atmega2560";
	uint32_t freq = 16000000;
	int debug = 0;
	int verbose = 1;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i] + strlen(argv[i]) - 4, ".hex"))
			strncpy(boot_path, argv[i], sizeof(boot_path));
		else if (!strcmp(argv[i], "-d"))
			debug++;
		else if (!strcmp(argv[i], "-v"))
			verbose++;
		else {
			fprintf(stderr, "%s: invalid argument %s\n", argv[0], argv[i]);
			exit(1);
		}
	}

	avr = avr_make_mcu_by_name(mmcu);
	if (!avr) {
		fprintf(stderr, "%s: Error creating the AVR core\n", argv[0]);
		exit(1);
	}

	 uint8_t * boot = read_ihex_file(boot_path, &boot_size, &boot_base);
	 if (!boot) {
		fprintf(stderr, "%s: Unable to load %s\n", argv[0], boot_path);
		exit(1);
	 }
	 printf("%s booloader 0x%05x: %d bytes\n", mmcu, boot_base, boot_size);

	elf_firmware_t f;
	char path[256] = "MK3S.afx";
	//sprintf(path, "%s/%s", dirname(argv[0]), fname);
	//printf("Firmware pathname is %s\n", path);
	elf_read_firmware(path, &f);


	snprintf(flash_data.avr_flash_path, sizeof(flash_data.avr_flash_path),
			"Einsy_%s_flash.bin", mmcu);
	flash_data.avr_flash_fd = 0;
	snprintf(flash_data.avr_eeprom_path, sizeof(flash_data.avr_eeprom_path),
			"Einsy_%s_eeprom.bin", mmcu);
	flash_data.avr_flash_fd = 0;
	// register our own functions
	avr->custom.init = avr_special_init;
	avr->custom.deinit = avr_special_deinit;
	avr->custom.data = &flash_data;
	avr_init(avr);
	flash_data.avr_eeprom_fd = einsy_eeprom_load(avr, flash_data.avr_eeprom_path);

	avr_load_firmware(avr,&f);
	avr->frequency = freq;
	avr->vcc = 5000;
	avr->aref = 0;
	avr->avcc = 5000;

	memcpy(avr->flash + boot_base, boot, boot_size);
	printf("Boot base at:%u\n",boot_base);
	free(boot);
	avr->pc = boot_base;
	avr->reset_pc = boot_base;
	/* end of flash, remember we are writing /code/ */
	avr->codeend = avr->flashend;
	avr->log = 1 + verbose;

	// even if not setup at startup, activate gdb if crashing
	avr->gdb_port = 1234;
	if (debug) {
		avr->state = cpu_Stopped;
		avr_gdb_init(avr);
	}

	setupSerial();

	setupHeaters();

	setupLCD();

	// Setup PP
	button_init(avr, &hw.powerPanic,"PowerPanic");
	//avr_raise_irq(hwPowerPanic.irq + IRQ_BUTTON_OUT, 0);

	/*
	 * OpenGL init, can be ignored
	 */
	glutInit(&argc, argv);		/* initialize GLUT system */

	int w = 5 + hw.lcd.w * 6;
	int h = 5 + hw.lcd.h * 9;
	int pixsize = 4;

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(w * pixsize, h * pixsize);		/* width=400pixels height=500pixels */
	window = glutCreateWindow("Press 'q' to quit");	/* create window */

	initGL(w * pixsize, h * pixsize);

	pthread_t run;
	pthread_create(&run, NULL, avr_run_thread, NULL);

	glutMainLoop();

	pthread_join(run,NULL);
	printf("Done");

}
