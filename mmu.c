
#include "Util.h"
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
		if (mmu.bReset)
		{
			mmu.bReset = 0;
			avr_reset(mmu.avr);
		}
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
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef((width)*1.05/350,4,1);
		tmc2130_draw_glut(&mmu.Sel);
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(width*1.05/350,4,1);
		glTranslatef(0,10,0);
		tmc2130_draw_glut(&mmu.Idl);
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(width*1.05/350,4,1);
		glTranslatef(0,20,0);
		tmc2130_draw_position_glut(&mmu.Extr);
	glPopMatrix();
	glPushMatrix();
		glColor3f(0,0,0);
		glLoadIdentity();		
		glScalef(width*2.8/350,4,1);
		glTranslatef(0,30,0);
		glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
		glEnd();
		for (int i=0; i<5; i++)
		{
			drawLED_gl(&mmu.lRed[i]);
			glTranslatef(11,0,0);
			drawLED_gl(&mmu.lGreen[i]);
			glTranslatef(12,0,0);
		}
		drawLED_gl(&mmu.lFINDA);
	glPopMatrix();

	glutSwapBuffers();

}

void timerMMU(int i)
{
	glutTimerFunc(50, timerMMU, 0);
	displayMMU();
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
	glutTimerFunc(100, timerMMU, 0);

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	return 1;
}

static void onShiftLatch(struct avr_irq_t * irq, uint32_t value, void * param)
{
	// Just clock out the various pins to the drivers.
	hc595_data_t v;	
	v.raw = value;

	avr_raise_irq(mmu.Extr.irq + IRQ_TMC2130_DIR_IN, 	v.bits.b0);
	avr_raise_irq(mmu.Extr.irq + IRQ_TMC2130_ENABLE_IN, v.bits.b1);
	avr_raise_irq(mmu.Sel.irq + IRQ_TMC2130_DIR_IN, 	v.bits.b2);
	avr_raise_irq(mmu.Sel.irq + IRQ_TMC2130_ENABLE_IN, 	v.bits.b3);
	avr_raise_irq(mmu.Idl.irq + IRQ_TMC2130_DIR_IN, 	v.bits.b4);
	avr_raise_irq(mmu.Idl.irq + IRQ_TMC2130_ENABLE_IN,	v.bits.b5);
	avr_raise_irq(mmu.lGreen[0].irq + IRQ_LED_IN, 		v.bits.b6);
	avr_raise_irq(mmu.lRed[0].irq + IRQ_LED_IN, 		v.bits.b7);
	avr_raise_irq(mmu.lGreen[4].irq + IRQ_LED_IN, 		v.bits.b8);
	avr_raise_irq(mmu.lRed[4].irq + IRQ_LED_IN, 		v.bits.b9);
	avr_raise_irq(mmu.lGreen[3].irq + IRQ_LED_IN, 		v.bits.b10);
	avr_raise_irq(mmu.lRed[3].irq + IRQ_LED_IN, 		v.bits.b11);
	avr_raise_irq(mmu.lGreen[2].irq + IRQ_LED_IN, 		v.bits.b12);
	avr_raise_irq(mmu.lRed[2].irq + IRQ_LED_IN, 		v.bits.b13);
	avr_raise_irq(mmu.lGreen[1].irq + IRQ_LED_IN, 		v.bits.b14);
	avr_raise_irq(mmu.lRed[1].irq + IRQ_LED_IN, 		v.bits.b15);
}

static void setupMotors()
{
	//There's only one shift register here that's 16 bits wide.
	// There are two on the MMU but they're chained, to the same effect.
	hc595_init(mmu.avr, &mmu.shift);
	avr_irq_register_notify(mmu.shift.irq + IRQ_HC595_OUT, onShiftLatch,NULL);

	avr_ioport_external_t ex = {.name = 'F', .value=0, .mask=0b10011};
	avr_ioctl(mmu.avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(ex.name),&ex);

	avr_connect_irq(IOIRQ(mmu.avr, 'B',6), 			mmu.shift.irq + IRQ_HC595_IN_LATCH);
	avr_connect_irq(IOIRQ(mmu.avr, 'B',5), 			mmu.shift.irq + IRQ_HC595_IN_DATA);
	avr_connect_irq(IOIRQ(mmu.avr, 'C',7), 			mmu.shift.irq + IRQ_HC595_IN_CLOCK);

	tmc2130_init(mmu.avr, &mmu.Extr, 'P', 30); // Init takwrkes care of the SPI wiring.
	avr_connect_irq(	IOIRQ(mmu.avr,'C',6),		mmu.Extr.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	IOIRQ(mmu.avr,'B',4),		mmu.Extr.irq + IRQ_TMC2130_STEP_IN);

	tmc2130_init(mmu.avr, &mmu.Sel, 'S', 30); // Init takes care of the SPI wiring.
	avr_connect_irq(	IOIRQ(mmu.avr,'D',7),		mmu.Sel.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	IOIRQ(mmu.avr,'D',4),		mmu.Sel.irq + IRQ_TMC2130_STEP_IN);

	tmc2130_init(mmu.avr, &mmu.Idl, 'I', 30); // Init takes care of the SPI wiring.
	avr_connect_irq(	IOIRQ(mmu.avr,'B',7),		mmu.Idl.irq + IRQ_TMC2130_SPI_CSEL);
	avr_connect_irq(	IOIRQ(mmu.avr,'D',6),		mmu.Idl.irq + IRQ_TMC2130_STEP_IN);

}

void setupLEDs()
{
	
	mmu_buttons_init(mmu.avr, &mmu.buttons,5);

	for (int i=0; i<5; i++)
	{
		led_init(mmu.avr, &mmu.lRed[i], 0xFF0000FF, ' ');
		led_init(mmu.avr,&mmu.lGreen[i],0x00FF00FF, ' ');
	}
	led_init(mmu.avr, &mmu.lFINDA,0xFFCC00FF,'F');
	avr_connect_irq(IOIRQ(mmu.avr, 'F',6), mmu.lFINDA.irq + IRQ_LED_IN);
	avr_raise_irq(IOIRQ(mmu.avr,'F',6),0);
}

static void start_mmu_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
	//printf("MMU RESET: %02x\n",value);
    mmu_t *pMMU = (mmu_t*) param;
	if (!value && !pMMU->bStarted)
		mmu_start(pMMU);
    else if (irq->value && !value)
        pMMU->bReset = true;
}
int32_t imax =0, imin = 0;
static void mmu_FINDA_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
	float* posOut = (float*)(&value);
   	avr_raise_irq(IOIRQ(mmu.avr,'F',6),posOut[0]>24.0f);
	mmu_t *pMMU = (mmu_t*) param;
	// Reflect the distance out for IR sensor triggering.
	avr_raise_irq(pMMU->irq + IRQ_MMU_FEED_DISTANCE, value);
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


static const char * irq_names[IRQ_MMU_COUNT] = {
	[IRQ_MMU_FEED_DISTANCE] = ">MMU.feed_distance"
};

mmu_t* mmu_init(avr_t *host, avr_irq_t *irqReset)
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

	mmu.irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_MMU_COUNT, irq_names);

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
	for (int i=0; i<5; i++)
		avr_extint_set_strict_lvl_trig(avr,i,false);

    uart_pty_init(avr, &mmu.UART0);

	avr_irq_register_notify(irqReset, start_mmu_hook, &mmu);

    printf("MMU UART:\n");
    uart_pty_connect(&mmu.UART0,'1');

	setupMotors();
	setupLEDs();

	avr_irq_register_notify(mmu.Extr.irq + IRQ_TMC2130_POSITION_OUT, mmu_FINDA_hook, &mmu );

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
	printf("MMU_stop()\n");
    uart_pty_stop(&mmu.UART0);
    if (!this->bStarted)
        return;
    this->bQuit = true;
    pthread_join(this->run,NULL);
    
    printf("MMU Done\n");
}

