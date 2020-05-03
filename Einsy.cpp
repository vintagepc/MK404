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
#include <sys/select.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include "Util.h"
#include <errno.h>

#if __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <pthread.h>
#include <signal.h>

#define TEMP_SENSOR_0 5
#define TEMP_SENSOR_BED 1
#define TEMP_SENSOR_AMBIENT 2000

#define _TERMISTOR_TABLE(num) \
		temptable_##num
#define TERMISTOR_TABLE(num) \
		_TERMISTOR_TABLE(num)

#include "Util.h"
#include "sim_avr.h"
#include "avr_ioport.h"
#include "avr_spi.h"
#include "avr_eeprom.h"
#include "avr_timer.h"
#include "avr_extint.h"
#include "sim_elf.h"
#include "sim_hex.h"
#include "sim_gdb.h"
#include "avr_uart.h"
extern "C" {
#include "uart_pty.h"
//#include "hd44780_glut.h"
#include "thermistortables.h"
#include "sim_vcd_file.h"
#define __cppOld __cplusplus
#undef __cplusplus // Needed to dodge some unwanted includes...
#include "Firmware/eeprom.h"
#define __cplusplus __cppOld
#include "uart_logger.h"
#include "sd_card.h"
}

#include "Button.h"
#include "Einsy_EEPROM.h"
#include "Fan.h"
#include "HD44780GL.h"
#include "Heater.h"
#include "IRSensor.h"
#include "MMU2.h"
#include "PINDA.h"
#include "RotaryEncoder.h"
#include "Thermistor.h"
#include "TMC2130.h"
#include "VoltageSrc.h"
#include "w25x20cl.h"

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
};

int window;

int iScheme = 0;

uint32_t colors[8] = {
		0x02c5fbff, 0x8d7ff8ff, 0xFFFFFFff, 0x00000055,
		0x382200ff, 0x000000ff , 0xFF9900ff, 0x00000055
};
#undef TMC2130
struct hw_t {
	avr_t *mcu;
	HD44780GL lcd;
	RotaryEncoder encoder;
	Button *PowerPanic;
	uart_pty_t UART0, UART2;
	Thermistor tExtruder, tBed, tPinda, tAmbient;
	Fan *fExtruder,*fPrint;
	Heater *hExtruder, *hBed;
	w25x20cl spiFlash;
	sd_card_t sd_card;
	TMC2130 X, Y, Z, E;
    VoltageSrc *vMain, *vBed;
	IRSensor *IR;
	PINDA pinda = PINDA((float) X_PROBE_OFFSET_FROM_EXTRUDER, (float)Y_PROBE_OFFSET_FROM_EXTRUDER);
	uart_logger_t logger;
	MMU2 mmu;
	Einsy_EEPROM *EEPROM;
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

	hw.EEPROM->Save();
	
	hw.spiFlash.Save();
	
	sd_card_unmount_file (avr, &hw.sd_card);

	uart_pty_stop(&hw.UART0);

	if (hw.logger.fdOut)
		uart_logger_stop(&hw.logger);
	else
		uart_pty_stop(&hw.UART2);

	hw.mmu.Stop();
}

void displayCB(void)		/* function called whenever redisplay needed */
{
	//if (hd44780_get_flag(&hw.lcd, HD44780_FLAG_DIRTY)==0 && 
	//	hd44780_get_flag(&hw.lcd, HD44780_FLAG_CRAM_DIRTY == 0))
	//	return;
	glutSetWindow(window);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW); // Select modelview matrix
	glPushMatrix();
	glLoadIdentity(); // Start with an identity matrix
	glScalef(4, 4, 1);

	hw.lcd.Draw(colors[(4*iScheme) + 0], /* background */
			colors[(4*iScheme) + 1], /* character background */
			colors[(4*iScheme) + 2], /* text */
			colors[(4*iScheme) + 3] /* shadow */ );
	glPopMatrix();

	// Do something for the motors...
	float fX = (5 + hw.lcd.GetWidth()* 6)*4;
	float fY = (5 + hw.lcd.GetHeight() * 9);
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(fX/350,4,1);
		glTranslatef(0,fY,0);
		hw.X.Draw();
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();
		glScalef(fX/350,4,1);
		glTranslatef(0, fY +10,0);
		hw.Y.Draw();
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();
		glScalef(fX/350,4,1);
		glTranslatef(0,fY +20,0);
		hw.Z.Draw();
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();
		glScalef(fX/350,4,1);
		glTranslatef(0,fY +30,0);
		hw.E.Draw_Simple();
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
	hw.PowerPanic->Press(1);

	//depress encoder knob
	avr_raise_irq(hw.encoder.GetIRQ(RotaryEncoder::OUT_BUTTON), 0);
}

static void *
avr_run_thread(
		void * ignore)
{
	printf("Starting AVR execution...\n");
	int state = cpu_Running;
	while ((state != cpu_Done) && (state != cpu_Crashed)){
		if (guKey) {
			switch (guKey) {
				case 'w':
					printf("<");
					hw.encoder.Twist(RotaryEncoder::CCW_CLICK);
					break;
				case 's':
					printf(">");
					hw.encoder.Twist(RotaryEncoder::CW_CLICK);
					break;
				case 0xd:
					printf("ENTER pushed\n");
					hw.encoder.Push();
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
					hw.encoder.PushAndHold();
					break;
				case 'y':
					hw.pinda.ToggleSheet();
					break;
				case 'f':
					hw.IR->Toggle();
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
			hw.PowerPanic->Press(500);
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
	displayCB();
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

	return 1;
}

void setupLCD()
{
	hw.lcd.Init(avr);
	// D4-D7,
	avr_irq_t *irqLCD[4] = {DIRQLU(avr, LCD_PINS_D4),
							DIRQLU(avr, LCD_PINS_D5),
							DIRQLU(avr, LCD_PINS_D6),
							DIRQLU(avr, LCD_PINS_D7)};
	for (int i = 0; i < 4; i++) {
		hw.lcd.ConnectTo(HD44780::D4+i,irqLCD[i]);
		hw.lcd.ConnectFrom(irqLCD[i], HD44780::D4+i);
		
	}
	hw.lcd.ConnectFrom(DIRQLU(avr,LCD_PINS_RS), HD44780::RS);
	hw.lcd.ConnectFrom(DIRQLU(avr,LCD_PINS_ENABLE),HD44780::E);

	hw.lcd.ConnectFrom(DIRQLU(avr, LCD_BL_PIN), HD44780::BRIGHTNESS_IN);
	hw.lcd.ConnectFrom(DPWMLU(avr, LCD_BL_PIN), HD44780::BRIGHTNESS_PWM_IN);
	avr_ioport_external_t ex = {.name = PORT(LCD_BL_PIN), .mask = (1UL << PIN(LCD_BL_PIN)), .value = 0UL};
	avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(ex.name), &ex);

	hw.encoder.Init(avr);
	hw.encoder.ConnectTo(RotaryEncoder::OUT_A, DIRQLU(avr, BTN_EN2));
	hw.encoder.ConnectTo(RotaryEncoder::OUT_B, DIRQLU(avr, BTN_EN1));
	hw.encoder.ConnectTo(RotaryEncoder::OUT_BUTTON, DIRQLU(avr,BTN_ENC));

}

void setupSDcard(char * mmcu)
{
	sd_card_init (avr, &hw.sd_card);
	sd_card_attach (avr, &hw.sd_card, AVR_IOCTL_SPI_GETIRQ (0), DIRQLU(avr, SDSS));
	
	// wire up the SD present signal.
	avr_connect_irq(hw.sd_card.irq + IRQ_SD_CARD_PRESENT, DIRQLU(avr,SDCARDDETECT));
    avr_ioport_external_t ex;
    ex.value = 0;
    ex.name = PORT(SDCARDDETECT);
    ex.mask = 1<<PIN(SDCARDDETECT);
	avr_ioctl(avr,AVR_IOCTL_IOPORT_SET_EXTERNAL(ex.name),&ex);

	snprintf(hw.sd_card.filepath, sizeof(hw.sd_card.filepath), "Einsy_%s_SDcard.bin", mmcu);
	int mount_error = sd_card_mount_file (avr, &hw.sd_card, hw.sd_card.filepath, 128450560);

	if (mount_error != 0) {
		fprintf (stderr, "SD card image ‘%s’ could not be mounted (error %i).\n", hw.sd_card.filepath, mount_error);
		exit (2);
	}
}

// Helper for MMU IR sensor triggering.
static void mmu_irsensor_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	float *fVal = (float*)&value;
	hw.IR->Auto_Input(fVal[0]>400); // Trigger IR if MMU P pos > 400mm
}


void setupSerial(bool bConnectS0, uint8_t uiLog)
{
	uart_pty_init(avr, &hw.UART0);
	uart_pty_init(avr, &hw.UART2);


	hw.spiFlash.Init(avr, DIRQLU(avr, W25X20CL_PIN_CS));
	uart_logger_init(avr, &hw.logger);

	// Uncomment these to get a pseudoterminal you can connect to
	// using any serial terminal program. Will print to console by default.
	if (bConnectS0)
    	uart_pty_connect(&hw.UART0, '0');

	if (uiLog=='0' || uiLog == '2')
		uart_logger_connect(&hw.logger,uiLog);
	else
		uart_pty_connect(&hw.UART2, '2');
}

void setupHeaters()
{

		hw.tExtruder.Init(avr, TEMP_0_PIN);
		hw.tExtruder.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_0),
								sizeof(TERMISTOR_TABLE(TEMP_SENSOR_0)) / sizeof(short) / 2,
								OVERSAMPLENR);

		hw.tBed.Init(avr, TEMP_BED_PIN);
		hw.tBed.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_BED),
							sizeof(TERMISTOR_TABLE(TEMP_SENSOR_BED)) / sizeof(short) / 2,
							OVERSAMPLENR);

		// same table as bed.
		hw.tPinda.Init(avr, TEMP_PINDA_PIN);
		hw.tPinda.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_BED),
							sizeof(TERMISTOR_TABLE(TEMP_SENSOR_BED)) / sizeof(short) / 2,
							OVERSAMPLENR);

		hw.tAmbient.Init(avr, TEMP_AMBIENT_PIN);
		hw.tAmbient.SetTable((short*)TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT),
		 						sizeof(TERMISTOR_TABLE(TEMP_SENSOR_AMBIENT)) / sizeof(short) / 2,
		 						OVERSAMPLENR);		

		hw.fExtruder = new Fan(3300);
		hw.fExtruder->Init(avr, DIRQLU(avr, TACH_0), 	DIRQLU(avr, EXTRUDER_0_AUTO_FAN_PIN), 	DPWMLU(avr,EXTRUDER_0_AUTO_FAN_PIN));
		hw.fPrint = new Fan(5000);
		hw.fPrint->Init(avr, DIRQLU(avr, TACH_1), 	DIRQLU(avr, FAN_PIN),	DPWMLU(avr, FAN_PIN));

		hw.hBed = new Heater(0.25, 25, true);
		hw.hBed->Init(avr, NULL, DIRQLU(avr,HEATER_BED_PIN));
		hw.hBed->ConnectTo(hw.hBed->TEMP_OUT, hw.tBed.GetIRQ(Thermistor::TEMP_IN));

		hw.hExtruder = new Heater(1.5,25.0);
		hw.hExtruder->Init(avr, NULL, DIRQLU(avr, HEATER_0_PIN));
		hw.hExtruder->ConnectTo(hw.hBed->TEMP_OUT, hw.tExtruder.GetIRQ(Thermistor::TEMP_IN));
}

void setupVoltages()
{
	float fScale24v = 1.0f/26.097f; // Based on rSense voltage divider outputting 5v
    hw.vBed = new VoltageSrc(VOLT_BED_PIN,	fScale24v,	23.9);
    hw.vBed->Init(avr);
    hw.vMain = new VoltageSrc(VOLT_PWR_PIN,	fScale24v,	24.0);
    hw.vMain->Init(avr);
	hw.IR = new IRSensor(VOLT_IR_PIN);
	hw.IR->Init(avr);
	hw.IR->ConnectTo(IRSensor::DIGITAL_OUT, DIRQLU(avr, IR_SENSOR_PIN));
}

void setupDrivers()
{
	// Fake an external pullup on the diag pins so it can be detected:
	// Note we can't do this inside the init because it clobbers others, so only the last
	// one that you set would stick.

	uint8_t uiDiagMask = 1<<PIN(X_TMC2130_DIAG) | 1 << PIN(Y_TMC2130_DIAG) | 1 << PIN(Z_TMC2130_DIAG) | 1 << PIN(E0_TMC2130_DIAG);
	printf( "Diag mask: %02x\n",uiDiagMask);
    avr_ioport_external_t ex;
    ex.value = 0;
    ex.name = 'K';
    ex.mask = uiDiagMask;
	avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(ex.name), &ex);

	struct TMC2130::TMC2130_cfg_t cfg;
	cfg.iMaxMM = 255;
	cfg.cAxis = 'X';
	cfg.uiDiagPin = X_TMC2130_DIAG;

	hw.X.SetConfig(cfg);
	hw.X.Init(avr);
	hw.X.ConnectFrom(DIRQLU(avr,X_TMC2130_CS), 	TMC2130::SPI_CSEL);
	hw.X.ConnectFrom(DIRQLU(avr,X_DIR_PIN),		TMC2130::DIR_IN);
	hw.X.ConnectFrom(DIRQLU(avr,X_STEP_PIN),		TMC2130::STEP_IN);
	hw.X.ConnectFrom(DIRQLU(avr,X_ENABLE_PIN),		TMC2130::ENABLE_IN);

	cfg.uiStepsPerMM = 400;
	cfg.iMaxMM = 219;
	cfg.cAxis = 'Z';
	cfg.uiDiagPin = Z_TMC2130_DIAG;

	hw.Z.SetConfig(cfg);
	hw.Z.Init(avr);
	hw.Z.ConnectFrom(DIRQLU(avr,Z_TMC2130_CS), 	TMC2130::SPI_CSEL);
	hw.Z.ConnectFrom(DIRQLU(avr,Z_DIR_PIN),		TMC2130::DIR_IN);
	hw.Z.ConnectFrom(DIRQLU(avr,Z_STEP_PIN),		TMC2130::STEP_IN);
	hw.Z.ConnectFrom(DIRQLU(avr,Z_ENABLE_PIN),		TMC2130::ENABLE_IN);

	cfg.bInverted = true;
	cfg.uiStepsPerMM = 100;
	cfg.cAxis = 'Y';
	cfg.iMaxMM = 220;
	cfg.uiDiagPin = Y_TMC2130_DIAG;

	hw.Y.SetConfig(cfg);
	hw.Y.Init(avr);
	hw.Y.ConnectFrom(DIRQLU(avr,Y_TMC2130_CS), 	TMC2130::SPI_CSEL);
	hw.Y.ConnectFrom(DIRQLU(avr,Y_DIR_PIN),		TMC2130::DIR_IN);
	hw.Y.ConnectFrom(DIRQLU(avr,Y_STEP_PIN),		TMC2130::STEP_IN);
	hw.Y.ConnectFrom(DIRQLU(avr,Y_ENABLE_PIN),		TMC2130::ENABLE_IN);

	cfg.bHasNoEndStops = true;
	cfg.fStartPos = 0;
	cfg.uiStepsPerMM = 280;
	cfg.uiDiagPin = E0_TMC2130_DIAG;

	hw.E.SetConfig(cfg);
	hw.E.Init(avr);
	hw.E.ConnectFrom(DIRQLU(avr,E0_TMC2130_CS), 	TMC2130::SPI_CSEL);
	hw.E.ConnectFrom(DIRQLU(avr,E0_DIR_PIN),		TMC2130::DIR_IN);
	hw.E.ConnectFrom(DIRQLU(avr,E0_STEP_PIN),		TMC2130::STEP_IN);
	hw.E.ConnectFrom(DIRQLU(avr,E0_ENABLE_PIN),	TMC2130::ENABLE_IN);




	ex.mask = 1<<PIN(Z_MIN_PIN); // DIAG pins. 
	ex.name = PORT(Z_MIN_PIN); // Value should already be 0 from above.
	avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(ex.name), &ex);
	
	// TODO once the drivers are setup.
	hw.pinda.Init(avr, hw.X.GetIRQ(TMC2130::POSITION_OUT),  hw.Y.GetIRQ(TMC2130::POSITION_OUT),  hw.Z.GetIRQ(TMC2130::POSITION_OUT));
	hw.pinda.ConnectTo(PINDA::TRIGGER_OUT ,DIRQLU(avr, Z_MIN_PIN));


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


static void *
serial_pipe_thread(
		void * ignore)
{
	// Not much to see here, we just open the ports and shuttle characters back and forth across them.
	printf("Starting serial transfer thread...\n");
	int fdPort[2]; 
	fd_set fdsIn, fdsErr;
	unsigned char chrIn;
	int iLastFd = 0, iReadyRead, iChrRd;
	bool bQuit = false;
	if ((fdPort[0]=open(hw.UART2.pty.slavename, O_RDWR | O_NONBLOCK)) == -1)
	{
		fprintf(stderr, "Could not open %s.\n",hw.UART2.pty.slavename);
		perror(hw.UART2.pty.slavename);
		bQuit = true;
	}
	if ((fdPort[1]=open(hw.mmu.GetSerialPort(), O_RDWR | O_NONBLOCK)) == -1)
	{
		fprintf(stderr, "Could not open %s.\n",hw.mmu.GetSerialPort());
		perror(hw.mmu.GetSerialPort());
		bQuit = true;
	}
	if (fdPort[0]>fdPort[1])
		iLastFd = fdPort[0];
	else
		iLastFd = fdPort[1];
		
	while (!bQuit)
	{
		FD_ZERO(&fdsIn);
		FD_ZERO(&fdsErr);
		FD_SET(fdPort[0], &fdsIn);
		FD_SET(fdPort[1], &fdsIn);
		FD_SET(fdPort[0], &fdsErr);
		FD_SET(fdPort[1], &fdsErr);
		if ((iReadyRead = select(iLastFd+1,&fdsIn, NULL, &fdsErr,NULL))<0)
		{
			printf("Select ERR.\n");
			bQuit = true;
			break;
		}

		if (FD_ISSET(fdPort[0],&fdsIn))
		{
			while ((iChrRd = read(fdPort[0], &chrIn,1))>0)
			{
				write(fdPort[1],&chrIn,1);
			}
			if (iChrRd == 0 || (iChrRd<0 && errno != EAGAIN))
			{
				bQuit = true; 
				break;
			}
		}
		if (FD_ISSET(fdPort[1],&fdsIn))
		{
			while ((iChrRd = read(fdPort[1], &chrIn,1))>0)
			{
				write(fdPort[0],&chrIn,1);
			}
			if (iChrRd == 0 || (iChrRd<0 && errno != EAGAIN))
			{
				bQuit = true; 
				break;
			}
		}
		if (FD_ISSET(fdPort[0], &fdsErr) || FD_ISSET(fdPort[1], &fdsErr))
		{
			fprintf(stderr,"Exception reading PTY. Quit.\n");
			bQuit = true; 
			break;
		}
		
	}

	// cleanup.
	for (int i=0; i<2; i++)
		close(fdPort[i]);
	return 0;
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

void * glutThread(void* p)
{
    glutMainLoop();
    return NULL;
}

int main(int argc, char *argv[])
{

	bool bBootloader = false, bConnectS0 = false, bWait = false, bMMU = false;
	struct avr_flash flash_data;
	char boot_path[1024] = "stk500boot_v2_mega2560.hex";
	//char boot_path[1024] = "atmega2560_PFW.axf";
	uint32_t boot_base, boot_size;
    char mmcu[] = "atmega2560";
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
		else if (!strcmp(argv[i], "-m"))
			bMMU = true;
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
	char strXFlashPath[1024];
	snprintf(strXFlashPath, sizeof(strXFlashPath),
			"Einsy_%s_xflash.bin", mmcu);
	// register our own functions
	avr->custom.init = avr_special_init;
	avr->custom.deinit = avr_special_deinit;
	avr->custom.data = &flash_data;
	avr_init(avr);
	avr->reset = powerup_and_reset_helper;
	hw.EEPROM = new Einsy_EEPROM(avr, flash_data.avr_eeprom_path);
	hw.spiFlash.Load(strXFlashPath);

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
	hw.PowerPanic = new Button("PowerPanic");
	hw.PowerPanic->Init(avr);
	hw.PowerPanic->ConnectTo(Button::BUTTON_OUT, DIRQLU(avr,2)); // Note - PP is not defined in pins_einsy, it's an EXTINT.

	powerup_and_reset_helper(avr);

	/*	
	 * OpenGL init, can be ignored
	 */
	glutInit(&argc, argv);		/* initialize GLUT system */

	int w = 5 + hw.lcd.GetWidth() * 6;
	int h = 5 + hw.lcd.GetHeight() * 9;
	h+=40;
	int pixsize = 4;

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(w * pixsize, h * pixsize);		/* width=400pixels height=500pixels */
	window = glutCreateWindow("Prusa i3 MK404 (PRINTER NOT FOUND) ('q' quits)");	/* create window */

	initGL(w * pixsize, h * pixsize);

	// Note we can't directly connect the MMU or you'll get serial flow issues/lost bytes. 
	// The serial_pipe thread lets us reuse the UART_PTY code and its internal xon/xoff/buffers
	// rather than having to roll our own internal FIFO. As an added bonus you can tap the ports for debugging.
	if (bMMU)
	{
		hw.mmu.Init();
		hw.mmu.StartGL();
		hw.mmu.ConnectFrom(IOIRQ(avr,'J',5),MMU2::RESET);
		hw.IR->Set(IRSensor::IR_AUTO);
		avr_irq_register_notify(hw.mmu.GetIRQ(MMU2::FEED_DISTANCE), mmu_irsensor_hook, avr);
	}

	// Useful for getting serial pipes/taps setup, the node exists so you can
	// start socat (or whatever) without worrying about missing a window for something you need to do at boot.
	if (bWait) 
	{
		printf("Paused - press any key to resume execution\n");
		getchar();
	}

    pthread_t run[3];

	if (bMMU) // SPin up the serial pipe
        pthread_create(&run[2], NULL, serial_pipe_thread, NULL);

	pthread_create(&run[0], NULL, avr_run_thread, NULL);
    pthread_create(&run[1], NULL, glutThread, NULL);

	pthread_join(run[0],NULL);
	pthread_cancel(run[1]); // Kill the GL thread.
 	printf("Writing flash state...\n");
    avr_terminate(avr);
    printf("AVR finished.\n");
	if (bMMU)
        pthread_join(run[2],NULL);
   

	printf("Done");

}
