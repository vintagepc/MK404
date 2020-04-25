
#include "Macros.h"
#include "mmu.h"
#include "sim_avr.h"
#include <stdio.h>
#include "sim_hex.h"
#include <stdlib.h>
#include <string.h>
#include "avr_extint.h"
#include "avr_uart.h"
#include "avr_ioport.h"
#include "GL/glut.h"

struct mmu_t mmu;

int MMwindow, width, height;

static void *
avr_run_thread(
		void * ignore)
{
	printf("Starting MMU2 execution...\n");
	int state = cpu_Running;
	while ((state != cpu_Done) && (state != cpu_Crashed) && !mmu.bQuit){	
		//if (gbPrintPC)
		//	printf("PC: %x\n",mmu->pc);
		state = avr_run(mmu.avr);
	}
	avr_terminate(mmu.avr);
	printf("MMU finished.\n");
	return NULL;
}

void displayMMU()		/* function called whenever redisplay needed */
{
	glutSetWindow(MMwindow);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW); // Select modelview matrix
	glPushMatrix();
	glLoadIdentity(); // Start with an identity matrix
	glScalef(4, 4, 1);
	glPopMatrix();

	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(width/350,4,1);
		tmc2130_draw_glut(&mmu.Sel);
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(width/350,4,1);
		glTranslatef(0,10,0);
		tmc2130_draw_glut(&mmu.Idl);
	glPopMatrix();
		glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(width/350,4,1);
		glTranslatef(0,20,0);
		tmc2130_draw_glut(&mmu.Extr);
	glPopMatrix();

	glutSwapBuffers();

}

// gl timer. if the lcd is dirty, refresh display
void timerMMU(int i)
{
	//static int oldstate = -1;
	// restart timer
	glutTimerFunc(100, timerMMU, 0);
	glutPostRedisplay();
	//hd44780_print(&hd44780);
}

int initMMUGL(int w, int h)
{
	// Set up projection matrix
	glMatrixMode(GL_PROJECTION); // Select projection matrix
	glLoadIdentity(); // Start with an identity matrix
	glOrtho(0, w, 0, h, 0, 10);
	glScalef(1,-1,1);
	glTranslatef(0, -1 * h, 0);

	glutDisplayFunc(displayMMU);		/* set window's display callback */
	glutTimerFunc(1000, timerMMU, 0);

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	return 1;
}

static void onMotorLatch(struct avr_irq_t * irq, uint32_t value, void * param)
{
	//printf("Latch %02x\n",value & 0xff);
	// Just clock out the various pins to the drivers.
	avr_raise_irq(mmu.Sel.irq + IRQ_TMC2130_DIR_IN, value & 1);
	avr_raise_irq(mmu.Sel.irq + IRQ_TMC2130_ENABLE_IN, value>>2 & 1);
	avr_raise_irq(mmu.Idl.irq + IRQ_TMC2130_DIR_IN, value>>3 & 1);
	avr_raise_irq(mmu.Idl.irq + IRQ_TMC2130_ENABLE_IN, value >> 4 & 1);
	avr_raise_irq(mmu.Extr.irq + IRQ_TMC2130_DIR_IN, value >>5 &1);
	avr_raise_irq(mmu.Extr.irq + IRQ_TMC2130_ENABLE_IN, value>>6 &1);
}

static void setupMotors()
{
	hc595_init(mmu.avr, &mmu.shiftMotors);

	avr_irq_register_notify(mmu.shiftMotors.irq + IRQ_HC595_OUT, onMotorLatch,NULL);

	avr_connect_irq(IOIRQ(mmu.avr, 'D',6), 			mmu.shiftMotors.irq + IRQ_HC595_IN_LATCH);
	avr_connect_irq(IOIRQ(mmu.avr, 'B',5), 			mmu.shiftMotors.irq + IRQ_HC595_IN_DATA);
	avr_connect_irq(IOIRQ(mmu.avr, 'C',7), 			mmu.shiftMotors.irq + IRQ_HC595_IN_CLOCK);

	tmc2130_init(mmu.avr, &mmu.Sel, 'S', 30); // Init takwrkes care of the SPI wiring.
	avr_connect_irq(	IOIRQ(mmu.avr,'C',6),		mmu.Sel.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	IOIRQ(mmu.avr,'B',4),		mmu.Sel.irq + IRQ_TMC2130_STEP_IN);

	tmc2130_init(mmu.avr, &mmu.Idl, 'I', 30); // Init takes care of the SPI wiring.
	avr_connect_irq(	IOIRQ(mmu.avr,'D',7),		mmu.Idl.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	IOIRQ(mmu.avr,'D',4),		mmu.Idl.irq + IRQ_TMC2130_STEP_IN);

	tmc2130_init(mmu.avr, &mmu.Extr, 'E', 30); // Init takes care of the SPI wiring.
	avr_connect_irq(	IOIRQ(mmu.avr,'B',7),		mmu.Extr.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	IOIRQ(mmu.avr,'D',6),		mmu.Extr.irq + IRQ_TMC2130_STEP_IN);

}

static void start_mmu_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
return;
	printf("MMU RESET: %02x\n",value);
    mmu_t *pMMU = (mmu_t*) param;
	if (value==0)
		mmu_start(pMMU);
    else
        printf("MMU RESET (TODO)\n");
}

static void mmu_tx_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
    //printf("MMU said: %02x\n",value);
    avr_irq_t *pHostRX = (avr_irq_t*) param;
    avr_raise_irq(pHostRX,value);
}

static void mmu_host_tx_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
    //printf("Host said: %02x\n",value);
    mmu_t *pMMU = (mmu_t*) param;
    avr_raise_irq(avr_io_getirq(pMMU->avr, AVR_IOCTL_UART_GETIRQ('1'), UART_IRQ_INPUT),value);
}

void mmu_startGL(mmu_t* this)
{
	int pixsize = 4;
	width = 20*6 * pixsize;
	height = 40*pixsize;
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(width,height);		/* width=400pixels height=500pixels */
	MMwindow = glutCreateWindow("Missing Material Unit 2");	/* create window */
	initMMUGL(width,height);
}


mmu_t* mmu_init(avr_t *host, avr_irq_t *hostTX, avr_irq_t *hostRX,  avr_irq_t *irqReset)
{
	uint32_t boot_base, boot_size;
	char * mmcu = "atmega32u4";
	uint32_t freq = 16000000;

	avr_t *avr = avr_make_mcu_by_name(mmcu);
    mmu.bQuit = false;
    mmu.bStarted = false;
    mmu.avr = avr;

	if (!avr) {
		fprintf(stderr, "Error creating the MMU2 core\n");
		return NULL;
	}
    char boot_path[] = "MM-control-01.hex";
	 uint8_t * boot = read_ihex_file(boot_path, &boot_size, &boot_base);
	 if (!boot) {
		fprintf(stderr, "MMU2: Unable to load %s\n", boot_path);
		return NULL;
	 }
	 printf("%s f/w 0x%05x: %d bytes\n", mmcu, boot_base, boot_size);
	avr_init(avr);

	avr->frequency = freq;
	avr->vcc = 5000;
	avr->aref = 0;
	avr->avcc = 5000;
	memcpy(avr->flash + boot_base, boot, boot_size);
	printf("fw base at:%u\n",boot_base);
	free(boot);
    /* end of flash, remember we are writing /code/ */
	avr->codeend = avr->flashend;
	avr->log = 1;

	// even if not setup at startup, activate gdb if crashing
	avr->gdb_port = 1234;
	
	// suppress continuous polling for low INT lines... major performance drain.
	for (int i=0; i<8; i++)
		avr_extint_set_strict_lvl_trig(avr,i,false);

    uart_pty_init(avr, &mmu.UART0);

	avr_irq_register_notify(irqReset, start_mmu_hook, &mmu);
    //avr_irq_register_notify(hostTX, mmu_host_tx_hook, &mmu);

    //avr_irq_register_notify(avr_io_getirq(mmu.avr, AVR_IOCTL_UART_GETIRQ('1'), UART_IRQ_OUTPUT), mmu_tx_hook, hostRX);


    printf("MMU UART:\n");
    uart_pty_connect(&mmu.UART0,'1');

	setupMotors();

    return &mmu;
}
void mmu_start(mmu_t *this)
{
    if (this->bStarted)
        return; 
    printf("Starting MMU...\n");
    this->bStarted = true;
	pthread_create(&mmu.run, NULL, avr_run_thread, NULL);
}

void mmu_stop(mmu_t *this){
    uart_pty_stop(&mmu.UART0);
    if (!this->bStarted)
        return;
    this->bQuit = true;
    pthread_join(this->run,NULL);
    
    printf("MMU Done\n");
}

