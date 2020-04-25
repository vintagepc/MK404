/*
	TMC2130.c

    Simulates a TMC2130 driver for virtualizing Marlin on simAVR.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "avr_ioport.h"
#include "sim_avr.h"
#include "avr_spi.h"
#include "avr_adc.h"
#include "TMC2130.h"
#include "stdbool.h"
#include "GL/glut.h"
#include "Macros.h"
//#define TRACE(_w) _w
#define TRACE2(_w) if (this->axis=='I') _w
#ifndef TRACE
#define TRACE(_w)
#endif


void tmc2130_draw_glut(tmc2130_t *this)
{
        if (!this->iStepsPerMM)
            return; // Motors not ready yet.
        float fEnd = this->iMaxPos/this->iStepsPerMM;
	    glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
		glEnd();
        glColor3f(1,1,1);
        if (this->flags.bits.enable)
        {
            glBegin(GL_QUADS);
                glVertex3f(3,8,0);
                glVertex3f(13,8,0);
                glVertex3f(13,1,0);
                glVertex3f(3,1,0);
            glEnd();
            glColor3f(0,0,0);
        }   
        glPushMatrix();
            glTranslatef(3,7,0);
            glScalef(0.09,-0.05,0);
            glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,this->axis);
        glPopMatrix();
        glColor3f(1,1,1);
        glPushMatrix();
            glTranslatef(280,7,0);
            glScalef(0.09,-0.05,0);
            char pos[7];
            sprintf(pos,"%3.02f",this->fCurPos);
            for (int i=0; i<7; i++)
                glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,pos[i]);

        glPopMatrix();
		glPushMatrix();
			glTranslatef(20,0,0);
			glColor3f(1,0,0);
			glBegin(GL_QUADS);
				glVertex3f(0,2,0);
				glVertex3f(-2,2,0);
				glVertex3f(-2,8,0);
				glVertex3f(0,8,0);
			glEnd();
			glBegin(GL_QUADS);
				glVertex3f(fEnd,2,0);
				glVertex3f(fEnd+2,2,0);
				glVertex3f(fEnd+2,8,0);
				glVertex3f(fEnd,8,0);
			glEnd();
			glColor3f(0,1,1);
			glBegin(GL_QUADS);
				glVertex3f(this->fCurPos-0.5,2,0);
				glVertex3f(this->fCurPos+0.5,2,0);
				glVertex3f(this->fCurPos+0.5,8,0);
				glVertex3f(this->fCurPos-0.5,8,0);
			glEnd();
		glPopMatrix();
}


void tmc2130_draw_position_glut(tmc2130_t *this)
{
        if (!this->iStepsPerMM)
            return; // Motors not ready yet.
        float fEnd = this->iMaxPos/this->iStepsPerMM;
	    glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
		glEnd();
        glColor3f(1,1,1);
        glPushMatrix();
            glTranslatef(3,7,0);
            glScalef(0.09,-0.05,0);
            glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,this->axis);
        glPopMatrix();
        glPushMatrix();
            glTranslatef(30,7,0);
            glScalef(0.09,-0.05,0);
            char pos[10];
            sprintf(pos,"%7.02f",this->fCurPos);
            for (int i=0; i<7; i++)
                glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,pos[i]);

        glPopMatrix();
}

static void tmc2130_create_reply(tmc2130_t *this)
{
    this->cmdOut.all = 0x00; // Copy over.
    if (this->cmdProc.bitsIn.RW == 0) // Last in was a read.
    {
        this->cmdOut.bitsOut.data = this->regs.raw[this->cmdProc.bitsIn.address];
        TRACE(printf("Reading out %x (%10lx)", this->cmdProc.bitsIn.address, this->cmdOut.bitsOut.data));
    }
    else
        this->cmdOut.bitsOut.data = this->cmdProc.bitsOut.data;
    // If the last was a write, the old data is left intact.

    // Set the status bits on the reply:
    this->cmdOut.bitsOut.driver_error = this->regs.defs.GSTAT.drv_err;
    this->cmdOut.bitsOut.reset_flag = this->regs.defs.GSTAT.reset;
    this->cmdOut.bitsOut.sg2 = this->regs.defs.DRV_STATUS.stallGuard;
    this->cmdOut.bitsOut.standstill = this->regs.defs.DRV_STATUS.stst;
    //(printf("Reply built: %10lx\n",this->cmdOut.all));
}

// Called when a full command is ready to process. 
static void tmc2130_process_command(tmc2130_t *this)
{
    TRACE(printf("tmc2130 %c cmd: w: %x a: %02x  d: %08x\n",this->axis, this->cmdProc.bitsIn.RW, this->cmdProc.bitsIn.address, this->cmdProc.bitsIn.data));
    if (this->cmdProc.bitsIn.RW)
    {
        this->regs.raw[this->cmdProc.bitsIn.address] = this->cmdProc.bitsIn.data;
        //printf("REG %c %02x set to: %010x\n", this->axis, this->cmdIn.bitsIn.address, this->cmdIn.bitsIn.data);
    }
    else
    {
        TRACE(printf("Read command on register: %02x\n", this->cmdProc.bitsIn.address));
    }
    tmc2130_create_reply(this);

}

/*
 * called when a SPI byte is received
 */
static void tmc2130_spi_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    tmc2130_t* this = (tmc2130_t*)param;
    if (!this->flags.bits.selected)
        return;
    // Clock out a reply byte, MSB first
    uint8_t byte = this->cmdOut.bytes[4];
    this->cmdOut.all<<=8;
    avr_raise_irq(this->irq + IRQ_TMC2130_SPI_BYTE_OUT,byte);
    TRACE(printf("TMC2130 %c: Clocking (%10lx) out %02x\n",this->axis,this->cmdOut.all,byte));

    this->cmdIn.all<<=8; // Shift bits up
    this->cmdIn.bytes[0] = value;
    TRACE(printf("TMC2130 %c: byte received: %02x (%010lx)\n",this->axis,value, this->cmdIn.all));
    


}

static void tmc2130_check_diag(tmc2130_t *this)
{
    bool bDiag = this->regs.defs.DRV_STATUS.stallGuard && this->regs.defs.GCONF.diag0_stall;
    //printf("Diag: %01x\n",bDiag);
    if (bDiag)
        avr_raise_irq(this->irq + IRQ_TMC2130_DIAG_OUT, bDiag^ this->regs.defs.GCONF.diag0_int_pushpull);
}

// Called when CSEL changes.
static void tmc2130_csel_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    tmc2130_t* this = (tmc2130_t*)param;
	TRACE(printf("TMC2130 %c: CSEL changed to %02x\n",this->axis,value));
    this->flags.bits.selected = (value==0); // NOTE: active low!
    if (value == 1) // Just finished a CSEL
    {
        this->cmdProc = this->cmdIn;
        tmc2130_process_command(this);
    }
}

// Called when DIR pin changes.
static void tmc2130_dir_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    if (irq->value == value)
        return;
    tmc2130_t* this = (tmc2130_t*)param;
	TRACE2(printf("TMC2130 %c: DIR changed to %02x\n",this->axis,value));
    this->flags.bits.dir = value^this->flags.bits.inverted; // XOR
}

// Called when STEP is triggered.
static void tmc2130_step_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    if (!value || irq->value) return; // Only step on rising pulse
    tmc2130_t* this = (tmc2130_t*)param;
    if (!this->flags.bits.enable) return;
	//TRACE2(printf("TMC2130 %c: STEP changed to %02x\n",this->axis,value));
    if (this->flags.bits.dir)    
        this->iCurStep--;
    else
        this->iCurStep++;
    bool bStall = false;
    if (this->iCurStep==-1 && this->axis != 'E')
    {
        this->iCurStep = 0;
        bStall = true;
    }
    else if (this->iCurStep>this->iMaxPos && this->axis != 'E')
    {
        this->iCurStep = this->iMaxPos;
        bStall = true;
    }
//    // TODO: get rid of this hackery once there's a real PINDA.
//    if (this->iCurStep==200)
//           avr_raise_irq(this->irq + IRQ_TMC2130_MIN_OUT, 1);
//    else if (this->iCurStep == 201)
//           avr_raise_irq(this->irq + IRQ_TMC2130_MIN_OUT, 0);

    this->fCurPos = (float)this->iCurStep/(float)this->iStepsPerMM;
    uint32_t* posOut = (uint32_t*)(&this->fCurPos); // both 32 bits, just mangle it for sending over the wire.
    avr_raise_irq(this->irq + IRQ_TMC2130_POSITION_OUT, posOut[0]);
    TRACE2(printf("cur pos: %f (%u)\n",this->fCurPos,this->iCurStep));
    if (bStall)
        avr_raise_irq(this->irq + IRQ_TMC2130_DIAG_OUT, 1);
    else if (!bStall && this->regs.defs.DRV_STATUS.stallGuard)
          avr_raise_irq(this->irq + IRQ_TMC2130_DIAG_OUT, 0);
    this->regs.defs.DRV_STATUS.stallGuard = bStall;
    //tmc2130_check_diag(this);
}

// Called when DRV_EN is triggered.
static void tmc2130_enable_in_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
    if (irq->value == value)
        return;
    tmc2130_t* this = (tmc2130_t*)param;
	TRACE2(printf("TMC2130 %c: EN changed to %02x\n",this->axis,value));
    this->flags.bits.enable = value==0; // active low, i.e motors off when high.
}

static const char * irq_names[IRQ_TMC2130_COUNT] = {
		[IRQ_TMC2130_SPI_BYTE_IN] = "8<tmc2130.spi.in",
		[IRQ_TMC2130_SPI_BYTE_OUT] = "8>tmc2130.chain",
        [IRQ_TMC2130_SPI_COMMAND_IN] = "40<tmc2130.cmd",
        [IRQ_TMC2130_SPI_CSEL] = "<tmc2130.csel",
        [IRQ_TMC2130_STEP_IN] = "<tmc2130.step",
        [IRQ_TMC2130_DIR_IN] = "<tmc2130.dir",
        [IRQ_TMC2130_ENABLE_IN] = "<tmc2130.enable",
        [IRQ_TMC2130_DIAG_OUT] = ">tmc2130.diag",
        [IRQ_TMC2130_POSITION_OUT] = ">tmc2130.position",
        [IRQ_TMC2130_MIN_OUT] = ">tmc2130.min",
};

void
tmc2130_init(
		struct avr_t * avr,
		tmc2130_t *this,
        char axis, uint8_t uiDiagPin) 
{
	this->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_TMC2130_COUNT, irq_names);
    this->axis = axis;
    memset(&this->cmdIn, 0, sizeof(this->cmdIn));
    memset(&this->regs.raw, 0, sizeof(this->regs.raw));
    this->fCurPos =15.0f; // start position.
    int iMaxMM = -1;
    // TODO: get steps/mm from the EEPROM?
    switch (axis)
    {
        case 'Y':
            this->flags.bits.inverted = 1;
            this->iStepsPerMM = 100;
            iMaxMM = 220;
            break;
        case 'X':
            this->iStepsPerMM = 100;
            iMaxMM = 255;
            break;
        case 'Z': 
            this->iStepsPerMM = 400;
            iMaxMM = 219;
            break;
        case 'E':
            this->flags.bits.inverted = 1;
            this->fCurPos = 0.0f;
            this->iStepsPerMM = 280;
            break;
        default:
            this->fCurPos = 0.0f;
            this->iStepsPerMM = 400;
            iMaxMM = 120;
    }
    if (iMaxMM==-1)
        this->iMaxPos = -1;
    else
        this->iMaxPos = iMaxMM*this->iStepsPerMM;

    this->iCurStep = this->fCurPos*this->iStepsPerMM; // We track in "steps" to avoid the cumulative floating point error of adding fractions of a mm to each pos.

    this->regs.defs.DRV_STATUS.SG_RESULT = 250;

    //for (int i=0; i<128; i++)
    //{
    //    if (this->regs.raw[i])
    //        printf("REG DUMP: %02x : %10lx",i,this->regs.raw[i]);
    //}

    // Just wire right up to the AVR SPI
    avr_connect_irq(avr_io_getirq(avr,AVR_IOCTL_SPI_GETIRQ(0),SPI_IRQ_OUTPUT),
		this->irq + IRQ_TMC2130_SPI_BYTE_IN);
    avr_connect_irq(this->irq + IRQ_TMC2130_SPI_BYTE_OUT,
        avr_io_getirq(avr,AVR_IOCTL_SPI_GETIRQ(0),SPI_IRQ_INPUT));
    
	avr_irq_register_notify(this->irq + IRQ_TMC2130_SPI_BYTE_IN, tmc2130_spi_in_hook, this);
    avr_irq_register_notify(this->irq + IRQ_TMC2130_SPI_CSEL, tmc2130_csel_in_hook, this);
    avr_irq_register_notify(this->irq + IRQ_TMC2130_DIR_IN, tmc2130_dir_in_hook, this);
    avr_irq_register_notify(this->irq + IRQ_TMC2130_STEP_IN, tmc2130_step_in_hook, this);
    avr_irq_register_notify(this->irq + IRQ_TMC2130_ENABLE_IN, tmc2130_enable_in_hook, this);
    avr_connect_irq(this->irq + IRQ_TMC2130_DIAG_OUT, DIRQLU(avr, uiDiagPin));	

    avr_raise_irq(this->irq + IRQ_TMC2130_DIAG_OUT,0);

}

