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
#include "Macros.h"

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

#include "Macros.h"
#include "sim_avr.h"
#include "avr_ioport.h"
#include "avr_spi.h"
#include "avr_eeprom.h"
#include "avr_timer.h"
#include "avr_extint.h"
#include "sim_elf.h"
#include "sim_hex.h"
#include "sim_gdb.h"
#include "uart_pty.h"
#include "hd44780_glut.h"
#include "PINDA.h"
#include "fan.h"
#include "heater.h"
#include "rotenc.h"
#include "button.h"
#include "voltage.h"
#include "IRSensor.h"
#include "thermistor.h"
#include "thermistortables.h"
#include "sim_vcd_file.h"
#include "w25x20cl.h"
#include "TMC2130.h"
#include "Firmware/eeprom.h"
#include "Einsy_EEPROM.h"
#include "uart_logger.h"
#include "stdbool.h"
#include "sd_card.h"
#include "Firmware/Configuration_prusa.h"

#define __AVR_ATmega2560__
#include "Firmware/pins_Einsy_1_0.h"

#include "Firmware/config.h"

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

int iScheme = 0;

uint32_t colors[8] = {
		0x02c5fbff, 0x8d7ff8ff, 0xFFFFFFff, 0x00000055,
		0x382200ff, 0x000000ff , 0xFF9900ff, 0x00000055
};

struct hw_t {
	avr_t *mcu;
	hd44780_t lcd;
	rotenc_t encoder;
	button_t powerPanic;
	uart_pty_t UART0, UART1, UART2, UART3;
	thermistor_t tExtruder, tBed, tPinda, tAmbient;
	fan_t fExtruder,fPrint;
	heater_t hExtruder, hBed;
	w25x20cl_t spiFlash;
	sd_card_t sd_card;
	tmc2130_t X, Y, Z, E;
	voltage_t vMain, vBed;
	irsensor_t IR;
	pinda_t pinda;
	uart_logger_t logger;
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

	// Do the same for the EEPROM
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
	
	w25x20cl_save(hw.spiFlash.filepath, &hw.spiFlash);
	
	sd_card_unmount_file (avr, &hw.sd_card);

	uart_pty_stop(&hw.UART0);

	if (hw.logger.fdOut)
		uart_logger_stop(&hw.logger);
	//uart_pty_stop(&hw.UART1);
}

void displayCB(void)		/* function called whenever redisplay needed */
{
	//if (hd44780_get_flag(&hw.lcd, HD44780_FLAG_DIRTY)==0 && 
	//	hd44780_get_flag(&hw.lcd, HD44780_FLAG_CRAM_DIRTY == 0))
	//	return;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW); // Select modelview matrix
	glPushMatrix();
	glLoadIdentity(); // Start with an identity matrix
	glScalef(4, 4, 1);

	hd44780_gl_draw(
		&hw.lcd,
			colors[(4*iScheme) + 0], /* background */
			colors[(4*iScheme) + 1], /* character background */
			colors[(4*iScheme) + 2], /* text */
			colors[(4*iScheme) + 3] /* shadow */ );
	glPopMatrix();

	// Do something for the motors...
	float fX = (5 + hw.lcd.w * 6)*4;
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(fX/350,4,1);
		glTranslatef(0,5 + hw.lcd.h * 9,0);
		tmc2130_draw_glut(&hw.X);
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();
		glScalef(fX/350,4,1);
		glTranslatef(0,(5 + hw.lcd.h * 9) +10,0);
		tmc2130_draw_glut(&hw.Y);
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();
		glScalef(fX/350,4,1);
		glTranslatef(0,(5 + hw.lcd.h * 9) +20,0);
		tmc2130_draw_glut(&hw.Z);
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();
		glScalef(fX/350,4,1);
		glTranslatef(0,(5 + hw.lcd.h * 9) +30,0);
		tmc2130_draw_position_glut(&hw.E);
	glPopMatrix();
	glutSwapBuffers();
}


// This is for stuff that needs to happen before powerup and after resets...
// things like resetting button states since that cancels any pending cycle timers.
void powerup_and_reset_helper(avr_t *avr)
{
	printf("RESET\n");
	// suppress continuous polling for low INT lines... major performance drain.
	for (int i=0; i<8; i++)
		avr_extint_set_strict_lvl_trig(avr,i,false);

	// Restore powerpanic to high
	avr_raise_irq(hw.powerPanic.irq + IRQ_BUTTON_OUT, 1);

	//depress encoder knob
	avr_raise_irq(hw.encoder.irq + IRQ_ROTENC_OUT_BUTTON_PIN, 0);
}

static void *
avr_run_thread(
		void * ignore)
{
	printf("Starting AVR execution...\n");
	int state = cpu_Running;
	float fNew;
	while ((state != cpu_Done) && (state != cpu_Crashed)){
		if (guKey) {
			switch (guKey) {
				case 'w':
					printf("<");
					rotenc_twist(&hw.encoder, ROTENC_CCW_CLICK);
					break;
				case 's':
					printf(">");
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
					break;
				case 't':
					printf("FACTORY_RESET\n");
					// Hold the button during boot to get factory reset menu
					avr_reset(avr);
					rotenc_button_press_hold(&hw.encoder);
					break;
				case 'y':
					hw.pinda.bIsSheetPresent ^=1;
					printf("Steel sheet: %s\n", hw.pinda.bIsSheetPresent? "INSTALLED" : "REMOVED");
					break;
				case 'f':
					if (hw.IR.eCurrent == IR_NO_FILAMENT)
					{
						printf("Filament PRESENT\n");
						irsensor_set(&hw.IR, IR_FILAMENT_PRESENT);
					}
					else
					{
						printf("Filament REMOVED\n");
						irsensor_set(&hw.IR, IR_NO_FILAMENT);
					}
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
	//printf("Keypress: %x\n",key);
	switch (key) {
		case 'q':
			//glutLeaveMainLoop();
			guKey = key;
			break;
		case 'p':
			printf("SIMULATING POWER PANIC\n");
			button_press(&hw.powerPanic, 500);
			break;
		case 'd':
			gbPrintPC = gbPrintPC==0;
			break;
		case '1':
			iScheme ^=1;
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
	glutTimerFunc(50, timerCB, 0);
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
	hd44780_init(avr, &hw.lcd, 20,4, LCD_BL_PIN);
	hd44780_set_flag(&hw.lcd, HD44780_FLAG_LOWNIBBLE, 0);
	// D4-D7,
	avr_irq_t *irqLCD[4] = {	DIRQLU(avr, LCD_PINS_D4),
							DIRQLU(avr, LCD_PINS_D5),
							DIRQLU(avr, LCD_PINS_D6),
							DIRQLU(avr, LCD_PINS_D7)};
	for (int i = 0; i < 4; i++) {
		avr_irq_t * ilcd = hw.lcd.irq + IRQ_HD44780_D4 + i;
		// AVR -> LCD
		avr_connect_irq(irqLCD[i], ilcd);
		// LCD -> AVR
		avr_connect_irq(ilcd, irqLCD[i]);
	}
	avr_connect_irq( DIRQLU(avr,LCD_PINS_RS), 		hw.lcd.irq + IRQ_HD44780_RS);
	avr_connect_irq( DIRQLU(avr, LCD_PINS_ENABLE),	hw.lcd.irq + IRQ_HD44780_E);

	rotenc_init(avr, &hw.encoder);
	avr_connect_irq(hw.encoder.irq + IRQ_ROTENC_OUT_A_PIN,		DIRQLU(avr, BTN_EN2));
	avr_connect_irq(hw.encoder.irq + IRQ_ROTENC_OUT_B_PIN,		DIRQLU(avr, BTN_EN1));
	avr_connect_irq(hw.encoder.irq + IRQ_ROTENC_OUT_BUTTON_PIN,	DIRQLU(avr,BTN_ENC));
}

void setupSDcard(char * mmcu)
{
	sd_card_init (avr, &hw.sd_card);
	sd_card_attach (avr, &hw.sd_card, AVR_IOCTL_SPI_GETIRQ (0), DIRQLU(avr, SDSS));
	
	// wire up the SD present signal.
	avr_connect_irq(hw.sd_card.irq + IRQ_SD_CARD_PRESENT, DIRQLU(avr,SDCARDDETECT));
	avr_ioport_external_t ex = {.name = PORT(SDCARDDETECT), .value = 0, .mask = 1<<PIN(SDCARDDETECT)};
	avr_ioctl(avr,AVR_IOCTL_IOPORT_SET_EXTERNAL(ex.name),&ex);

	snprintf(hw.sd_card.filepath, sizeof(hw.sd_card.filepath), "Einsy_%s_SDcard.bin", mmcu);
	int mount_error = sd_card_mount_file (avr, &hw.sd_card, hw.sd_card.filepath, 128450560);

	if (mount_error != 0) {
		fprintf (stderr, "SD card image ‘%s’ could not be mounted (error %i).\n", hw.sd_card.filepath, mount_error);
		exit (2);
	}
}

void setupSerial(bool bConnectS0, uint8_t uiLog)
{
	uart_pty_init(avr, &hw.UART0);
//	uart_pty_init(avr, &hw.UART1);
	uart_pty_init(avr, &hw.UART2);
//	uart_pty_init(avr, &hw.UART3);

	w25x20cl_init(avr, &hw.spiFlash, DIRQLU(avr, W25X20CL_PIN_CS));
	uart_logger_init(avr, &hw.logger);

	// Uncomment these to get a pseudoterminal you can connect to
	// using any serial terminal program. Will print to console by default.
	if (bConnectS0)
    	uart_pty_connect(&hw.UART0, '0');

	if (uiLog=='0' || uiLog == '2')
		uart_logger_connect(&hw.logger,uiLog);
	//uart_pty_connect(&hw.UART1,'1');
	//uart_pty_connect(&hw.UART0, '2');
	//uart_pty_connect(&hw.UART1,'3');
}

void setupHeaters()
{
	thermistor_init(avr, &hw.tExtruder, TEMP_0_PIN,
		(short*)TERMISTOR_TABLE(TEMP_SENSOR_0),
		sizeof(TERMISTOR_TABLE(TEMP_SENSOR_0)) / sizeof(short) / 2,
		OVERSAMPLENR, 25.0f);

		 thermistor_init(avr, &hw.tBed, TEMP_BED_PIN,
		 (short*)TERMISTOR_TABLE(TEMP_SENSOR_BED),
		 sizeof(TERMISTOR_TABLE(TEMP_SENSOR_BED)) / sizeof(short) / 2,
		 OVERSAMPLENR, 23.0f);

		// same table as bed.
		thermistor_init(avr, &hw.tPinda, TEMP_PINDA_PIN,
		 (short*)TERMISTOR_TABLE(TEMP_SENSOR_BED),
		 sizeof(TERMISTOR_TABLE(TEMP_SENSOR_BED)) / sizeof(short) / 2,
		 OVERSAMPLENR, 24.0f);

		thermistor_init(avr, &hw.tAmbient, TEMP_AMBIENT_PIN,
		 (short*)TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT),
		 sizeof(TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT)) / sizeof(short) / 2,
		 OVERSAMPLENR, 21.0f);		

		fan_init(avr, &hw.fExtruder,3300, 	DIRQLU(avr, TACH_0), 	DIRQLU(avr, EXTRUDER_0_AUTO_FAN_PIN), 	DPWMLU(avr,EXTRUDER_0_AUTO_FAN_PIN));
		fan_init(avr, &hw.fPrint,4500, 		DIRQLU(avr, TACH_1), 	DIRQLU(avr, FAN_PIN),					DPWMLU(avr, FAN_PIN));

		heater_init(avr, &hw.hBed, 0.25,25.0, NULL,			DIRQLU(avr,HEATER_BED_PIN));
		hw.hBed.bIsBed = true;

		heater_init(avr, &hw.hExtruder, 1.5, 25.0, NULL,	DIRQLU(avr, HEATER_0_PIN));

		avr_connect_irq(hw.hExtruder.irq + IRQ_HEATER_TEMP_OUT,hw.tExtruder.irq + IRQ_TERM_TEMP_VALUE_IN);
		avr_connect_irq(hw.hBed.irq + IRQ_HEATER_TEMP_OUT,hw.tBed.irq + IRQ_TERM_TEMP_VALUE_IN);
}

void setupVoltages()
{
	float fScale24v = 1.0f/26.097f; // Based on rSense voltage divider outputting 5v
	voltage_init(avr, &hw.vBed,		VOLT_BED_PIN,	fScale24v,	23.9);
	voltage_init(avr, &hw.vMain,	VOLT_PWR_PIN,	fScale24v,	24.0);
	irsensor_init(avr, &hw.IR,		VOLT_IR_PIN);
	irsensor_set(&hw.IR, IR_NO_FILAMENT);
	avr_connect_irq(hw.IR.irq + IRQ_IRSENSOR_DIGITAL_OUT, DIRQLU(avr,IR_SENSOR_PIN));
}

void setupDrivers()
{
	// Fake an external pullup on the diag pins so it can be detected:
	// Note we can't do this inside the init because it clobbers others, so only the last
	// one that you set would stick.

	uint8_t uiDiagMask = 1<<PIN(X_TMC2130_DIAG) | 1 << PIN(Y_TMC2130_DIAG) | 1 << PIN(Z_TMC2130_DIAG) | 1 << PIN(E0_TMC2130_DIAG);
	printf( "Diag mask: %02x\n",uiDiagMask);
    avr_ioport_external_t ex = {.value = 0, .name = 'K', .mask = uiDiagMask};
	avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(ex.name), &ex);

	tmc2130_init(avr, &hw.X, 'X', X_TMC2130_DIAG); // Init takes care of the SPI wiring.
	avr_connect_irq(	DIRQLU(avr,X_TMC2130_CS), 	hw.X.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	DIRQLU(avr,X_DIR_PIN),		hw.X.irq + IRQ_TMC2130_DIR_IN);
	avr_connect_irq(	DIRQLU(avr,X_STEP_PIN),		hw.X.irq + IRQ_TMC2130_STEP_IN);
	avr_connect_irq(	DIRQLU(avr,X_ENABLE_PIN),	hw.X.irq + IRQ_TMC2130_ENABLE_IN);


	tmc2130_init(avr, &hw.Y, 'Y', Y_TMC2130_DIAG); // Init takes care of the SPI wiring.
	avr_connect_irq(	DIRQLU(avr,Y_TMC2130_CS), 	hw.Y.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	DIRQLU(avr,Y_DIR_PIN),		hw.Y.irq + IRQ_TMC2130_DIR_IN);
	avr_connect_irq(	DIRQLU(avr,Y_STEP_PIN),		hw.Y.irq + IRQ_TMC2130_STEP_IN);
	avr_connect_irq(	DIRQLU(avr,Y_ENABLE_PIN),	hw.Y.irq + IRQ_TMC2130_ENABLE_IN);

	tmc2130_init(avr, &hw.Z, 'Z', Z_TMC2130_DIAG); // Init takes care of the SPI wiring.
	avr_connect_irq(	DIRQLU(avr,Z_TMC2130_CS), 	hw.Z.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	DIRQLU(avr,Z_DIR_PIN),		hw.Z.irq + IRQ_TMC2130_DIR_IN);
	avr_connect_irq(	DIRQLU(avr,Z_STEP_PIN),		hw.Z.irq + IRQ_TMC2130_STEP_IN);
	avr_connect_irq(	DIRQLU(avr,Z_ENABLE_PIN),	hw.Z.irq + IRQ_TMC2130_ENABLE_IN);

 	tmc2130_init(avr, &hw.E, 'E', E0_TMC2130_DIAG); // Init takes care of the SPI wiring.
	avr_connect_irq(	DIRQLU(avr,E0_TMC2130_CS), 	hw.E.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	DIRQLU(avr,E0_DIR_PIN),		hw.E.irq + IRQ_TMC2130_DIR_IN);
	avr_connect_irq(	DIRQLU(avr,E0_STEP_PIN),	hw.E.irq + IRQ_TMC2130_STEP_IN);
	avr_connect_irq(	DIRQLU(avr,E0_ENABLE_PIN),	hw.E.irq + IRQ_TMC2130_ENABLE_IN);


	ex.mask = 1<<PIN(Z_MIN_PIN); // DIAG pins. 
	ex.name = PORT(Z_MIN_PIN); // Value should already be 0 from above.
	avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(ex.name), &ex);
	
	pinda_init(avr, &hw.pinda ,X_PROBE_OFFSET_FROM_EXTRUDER, Y_PROBE_OFFSET_FROM_EXTRUDER,
		hw.X.irq + IRQ_TMC2130_POSITION_OUT, hw.Y.irq + IRQ_TMC2130_POSITION_OUT, hw.Z.irq + IRQ_TMC2130_POSITION_OUT);

	avr_connect_irq(hw.pinda.irq + IRQ_PINDA_TRIGGER_OUT ,DIRQLU(avr, Z_MIN_PIN));


}
void setupTimers(avr_t* avr)
{
	 avr_regbit_t rb = AVR_IO_REGBITS(0xB0,0,0xFF);
	 //	avr_regbit_setto(avr, rb, 0x03);
	//	rb.reg++;
	//avr_regbit_setto(avr, rb, 0x03); // B

	// rb.reg = 0x80; // TCCR1A
	// avr_regbit_setto(avr, rb, 0x01);
	// rb.reg++;
	// avr_regbit_setto(avr, rb, 0x03); // B

	// //rb.reg = 0xB0; // TCCR2A
	// avr_regbit_setto(avr, rb, 0);
	// rb.reg++;
	// avr_regbit_setto(avr, rb, 0x1); // B

	//  rb.reg=0xb3; //OCR2A
	//  avr_regbit_setto(avr, rb, 0x00);
	//  rb.reg++; // 2B
	//  avr_regbit_setto(avr, rb, 128);

	rb.reg = 0x6e;
	rb.mask= 0b00000111;
	// TIMSK0
//	avr_regbit_setto(avr,rb,0x01);

	rb.reg = 0x70;
	// TIMSK2
	avr_regbit_setto(avr,rb,0x01);

	//rb.reg++;
	//rb.mask= 0b00001111;
	// TIMSK3
	//avr_regbit_setto(avr,rb,0b00000010);

	//rb.reg = 0x90; // TCCR3A
	//avr_regbit_setto(avr, rb, 0x03);// | 1<<6);
	// rb.reg++;
	// avr_regbit_setto(avr, rb, 1); // B

//	 rb.reg = 0xA0; // TCCR4A
//	 avr_regbit_setto(avr, rb, 0x03);// | 1 <<6);
//	 rb.reg++;
//	 avr_regbit_setto(avr, rb, 0x01); // B

	// rb.reg = 0x120; // TCCR5A
	// avr_regbit_setto(avr, rb, 0x01);
	// rb.reg++;
	// avr_regbit_setto(avr, rb, 0x03); // B */

}



void fix_serial(avr_t * avr, avr_io_addr_t addr, uint8_t v, void * param)
{
	if (v==0x02)// Marlin is done setting up UCSRA0...
	{
		v|=(1<<5); // leave the UDRE0 alone
		printf("Reset UDRE0 after serial config changed\n");
	}
	avr_core_watch_write(avr,addr,v);	
}


int main(int argc, char *argv[])
{
	bool bBootloader = false, bConnectS0 = false, bWait = false;

	struct avr_flash flash_data;
	char boot_path[1024] = "stk500boot_v2_mega2560.hex";
	//char boot_path[1024] = "atmega2560_PFW.axf";
	uint32_t boot_base, boot_size;
	char * mmcu = "atmega2560";
	uint32_t freq = 16000000;
	int debug = 0;
	uint8_t chrLogSerial = ' ';
	int verbose = 1;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i] + strlen(argv[i]) - 4, ".hex"))
			strncpy(boot_path, argv[i], sizeof(boot_path));
		else if (!strcmp(argv[i], "-d"))
			debug++;
		else if (!strcmp(argv[i], "-v"))
			verbose++;
		else if (!strcmp(argv[i], "-b"))
			bBootloader = true;
		else if (!strcmp(argv[i], "-w"))
			bWait = true;
		else if (!strcmp(argv[i], "-S0"))
			bConnectS0 = true;
		else if (!strncmp(argv[i], "-l",2))
			chrLogSerial = argv[i][2];
		else {
			fprintf(stderr, "%s: invalid argument %s\n", argv[0], argv[i]);
			exit(1);
		}
	}

	avr = avr_make_mcu_by_name(mmcu);

	hw.mcu = avr;
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
	snprintf(hw.spiFlash.filepath, sizeof(hw.spiFlash.filepath),
			"Einsy_%s_xflash.bin", mmcu);
	hw.spiFlash.xflash_fd = 0;
	// register our own functions
	avr->custom.init = avr_special_init;
	avr->custom.deinit = avr_special_deinit;
	avr->custom.data = &flash_data;
	avr_init(avr);
	avr->reset = powerup_and_reset_helper;
	flash_data.avr_eeprom_fd = einsy_eeprom_load(avr, flash_data.avr_eeprom_path);
	hw.spiFlash.xflash_fd = w25x20cl_load(hw.spiFlash.filepath, &hw.spiFlash);

	avr_load_firmware(avr,&f);
	avr->frequency = freq;
	avr->vcc = 5000;
	avr->aref = 0;
	avr->avcc = 5000;
	memcpy(avr->flash + boot_base, boot, boot_size);
	printf("Boot base at:%u\n",boot_base);
	free(boot);
	if (bBootloader)
	{
		avr->pc = boot_base;
	}
	// Always set to bootloader or DTR/watchdog will definitely fail.
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

	

	setupSerial(bConnectS0, chrLogSerial);
	
	setupSDcard(mmcu);

	setupHeaters();

	setupLCD();

	setupDrivers();

	setupTimers(avr);

	setupVoltages();

	avr_register_io_write(avr,0xC0,fix_serial,(void*)NULL); // UCSRA0

	// Setup PP
	button_init(avr, &hw.powerPanic,"PowerPanic");
	avr_connect_irq(hw.powerPanic.irq + IRQ_BUTTON_OUT, DIRQLU(avr, 2)); // Note - PP is not defined in pins_einsy.

	// Useful for getting serial pipes/taps setup, the node exists so you can
	// start socat (or whatever) without worrying about missing a window for something you need to do at boot.
	if (bWait) 
	{
		printf("Paused - press any key to resume execution\n");
		getchar();
	}
	powerup_and_reset_helper(avr);

	/*
	 * OpenGL init, can be ignored
	 */
	glutInit(&argc, argv);		/* initialize GLUT system */

	int w = 5 + hw.lcd.w * 6;
	int h = 5 + hw.lcd.h * 9;
	h+=40;
	int pixsize = 4;

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(w * pixsize, h * pixsize);		/* width=400pixels height=500pixels */
	window = glutCreateWindow("Prusa i3 MK404 (PRINTER NOT FOUND) ('q' quits)");	/* create window */

	initGL(w * pixsize, h * pixsize);

	pthread_t run;
	pthread_create(&run, NULL, avr_run_thread, NULL);

	glutMainLoop();

	pthread_join(run,NULL);
	printf("Done");

}
