/*
	TMC2130.c

    Simulates a TMC2130 driver for virtualizing Marlin on simAVR.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "avr_ioport.h"
#include "avr_spi.h"
#include "TMC2130.h"
#include "GL/glut.h"
#include "Util.h"

//#define TRACE(_w) _w
#define TRACE2(_w) if (cfg.cAxis=='S' || cfg.cAxis=='I') _w
#ifndef TRACE
#define TRACE(_w)
#endif


void TMC2130::Draw()
{
        if (!m_pAVR)
            return; // Motors not ready yet.
        float fEnd = m_uiMaxPos/cfg.uiStepsPerMM;
	    glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
		glEnd();
        glColor3f(1,1,1);
        if (m_bEnable)
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
            glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,cfg.cAxis);
        glPopMatrix();
        glColor3f(1,1,1);
        glPushMatrix();
            glTranslatef(280,7,0);
            glScalef(0.09,-0.05,0);
            char pos[7];
            sprintf(pos,"%3.02f",m_fCurPos);
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
				glVertex3f(m_fCurPos-0.5,2,0);
				glVertex3f(m_fCurPos+0.5,2,0);
				glVertex3f(m_fCurPos+0.5,8,0);
				glVertex3f(m_fCurPos-0.5,8,0);
			glEnd();
		glPopMatrix();
}


void TMC2130::Draw_Simple()
{
        if (!cfg.uiStepsPerMM)
            return; // Motors not ready yet.
        float fEnd = cfg.iMaxMM/cfg.uiStepsPerMM;
	    glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
		glEnd();
        glColor3f(1,1,1);
         if (m_bEnable)
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
            glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,cfg.cAxis);
        glPopMatrix();
        glPushMatrix();
            glTranslatef(30,7,0);
            glScalef(0.09,-0.05,0);
            char pos[10];
            sprintf(pos,"%7.02f",m_fCurPos);
            for (int i=0; i<7; i++)
                glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,pos[i]);

        glPopMatrix();
}

void TMC2130::CreateReply()
{
    m_cmdOut.all = 0x00; // Copy over.
    if (m_cmdProc.bitsIn.RW == 0) // Last in was a read.
    {
        m_cmdOut.bitsOut.data = m_regs.raw[m_cmdProc.bitsIn.address];
        if (m_cmdProc.bitsIn.address == 0x01)
            m_regs.raw[0x01] = 0; // GSTAT is cleared after read.
        TRACE(printf("Reading out %x (%10lx)", m_cmdProc.bitsIn.address, m_cmdOut.bitsOut.data));
    }
    else
        m_cmdOut.bitsOut.data = m_cmdProc.bitsOut.data;
    // If the last was a write, the old data is left intact.

    // Set the status bits on the reply:
    m_cmdOut.bitsOut.driver_error = m_regs.defs.GSTAT.drv_err;
    m_cmdOut.bitsOut.reset_flag = m_regs.defs.GSTAT.reset;
    m_cmdOut.bitsOut.sg2 = m_regs.defs.DRV_STATUS.stallGuard;
    m_cmdOut.bitsOut.standstill = m_regs.defs.DRV_STATUS.stst;
    //(printf("Reply built: %10lx\n",m_cmdOut.all));
}

// Called when a full command is ready to process. 
void TMC2130::ProcessCommand()
{
    TRACE(printf("tmc2130 %c cmd: w: %x a: %02x  d: %08x\n",cfg.cAxis, m_cmdProc.bitsIn.RW, m_cmdProc.bitsIn.address, m_cmdProc.bitsIn.data));
    if (m_cmdProc.bitsIn.RW)
    {
        m_regs.raw[m_cmdProc.bitsIn.address] = m_cmdProc.bitsIn.data;
        //printf("REG %c %02x set to: %010x\n", cfg.cAxis, m_cmdIn.bitsIn.address, m_cmdIn.bitsIn.data);
    }
    else
    {
        TRACE(printf("Read command on register: %02x\n", m_cmdProc.bitsIn.address));
    }
    CreateReply();

}

/*
 * called when a SPI byte is received. It's guarded by the SPIPeripheral CSEL check.
 */
uint8_t TMC2130::OnSPIIn(struct avr_irq_t * irq, uint32_t value)
{
    m_cmdIn.all<<=8; // Shift bits up
    m_cmdIn.bytes[0] = value;
    TRACE(printf("TMC2130 %c: byte received: %02x (%010lx)\n",cfg.cAxis,value, m_cmdIn.all));
    // Clock out a reply byte, MSB first
    uint8_t byte = m_cmdOut.bytes[4];
    m_cmdOut.all<<=8;
    TRACE(printf("TMC2130 %c: Clocking (%10lx) out %02x\n",cfg.cAxis,m_cmdOut.all,byte));
    SetSendReplyFlag();
    return byte; // SPIPeripheral takes care of the reply.
}

void TMC2130::CheckDiagOut()
{
    bool bDiag = m_regs.defs.DRV_STATUS.stallGuard && m_regs.defs.GCONF.diag0_stall;
    //printf("Diag: %01x\n",bDiag);
    if (bDiag)
        RaiseIRQ(DIAG_OUT, bDiag^ m_regs.defs.GCONF.diag0_int_pushpull);
}

// Called when CSEL changes.
void TMC2130::OnCSELIn(struct avr_irq_t * irq, uint32_t value)
{
	TRACE(printf("TMC2130 %c: CSEL changed to %02x\n",cfg.cAxis,value));
    if (value == 1) // Just finished a CSEL
    {
        m_cmdProc = m_cmdIn;
        ProcessCommand();
    }
}

// Called when DIR pin changes.
void TMC2130::OnDirIn(struct avr_irq_t * irq, uint32_t value)
{
    if (irq->value == value)
        return;
    TRACE(printf("TMC2130 %c: DIR changed to %02x\n",cfg.cAxis,value));
    m_bDir = value^cfg.bInverted; // XOR
}

avr_cycle_count_t TMC2130::OnStandStillTimeout(avr_t *avr, avr_cycle_count_t when)
{
    m_regs.defs.DRV_STATUS.stst = true;
    return 0;
}

// Called when STEP is triggered.
void TMC2130::OnStepIn(struct avr_irq_t * irq, uint32_t value)
{
    if (!value || irq->value) return; // Only step on rising pulse
    if (!m_bEnable) return;
    CancelTimer(m_fcnStandstill,this);
	//TRACE2(printf("TMC2130 %c: STEP changed to %02x\n",cfg.cAxis,value));
    if (m_bDir)    
        m_iCurStep--;
    else
        m_iCurStep++;
    bool bStall = false;
    if (!cfg.bHasNoEndStops)
    {
        if (m_iCurStep==-1)
        {
            m_iCurStep = 0;
            bStall = true;
        }
        else if (m_iCurStep>m_uiMaxPos)
        {
            m_iCurStep = m_uiMaxPos;
            bStall = true;
        }
    }

    m_fCurPos = (float)m_iCurStep/(float)cfg.uiStepsPerMM;
    uint32_t* posOut = (uint32_t*)(&m_fCurPos); // both 32 bits, just mangle it for sending over the wire.
    RaiseIRQ(POSITION_OUT, posOut[0]);
    TRACE(printf("cur pos: %f (%u)\n",m_fCurPos,m_iCurStep));
    if (bStall)
    {
        RaiseIRQ(DIAG_OUT, 1);
        m_regs.defs.DRV_STATUS.SG_RESULT = 0;
    }
    else if (!bStall && m_regs.defs.DRV_STATUS.stallGuard)
    {
          RaiseIRQ(DIAG_OUT, 0);
          m_regs.defs.DRV_STATUS.SG_RESULT = 250;
    }
    m_regs.defs.DRV_STATUS.stallGuard = bStall;
    m_regs.defs.DRV_STATUS.stst = false;
    // 2^20 comes from the datasheet.
    RegisterTimer(m_fcnStandstill,2^20,this);
}

// Called when DRV_EN is triggered.
void TMC2130::OnEnableIn(struct avr_irq_t * irq, uint32_t value)
{
    if (irq->value == value && m_bEnable == (value==0))
        return;
	TRACE2(printf("TMC2130 %c: EN changed to %02x\n",cfg.cAxis,value));
    m_bEnable = value==0; // active low, i.e motors off when high.
}

TMC2130::TMC2130()
{
    memset(&m_regs.raw, 0, sizeof(m_regs.raw));
    m_regs.defs.DRV_STATUS.stst = true;
    m_regs.defs.DRV_STATUS.SG_RESULT = 250;
    m_regs.defs.GSTAT.reset = 1; // signal reset
}

void TMC2130::SetConfig(TMC2130_cfg_t cfgIn)
{
    cfg = cfgIn;
    m_iCurStep = cfg.fStartPos*cfg.uiStepsPerMM;
    m_uiMaxPos = cfg.iMaxMM*cfg.uiStepsPerMM;
    m_fCurPos = cfg.fStartPos;
}

void TMC2130::Init(struct avr_t * avr) 
{
    _Init(avr, this);

    RegisterNotify(DIR_IN,      MAKE_C_CALLBACK(TMC2130,OnDirIn), this);
    RegisterNotify(STEP_IN,     MAKE_C_CALLBACK(TMC2130,OnStepIn), this);
    RegisterNotify(ENABLE_IN,   MAKE_C_CALLBACK(TMC2130,OnEnableIn), this);
    ConnectTo(DIAG_OUT, DIRQLU(avr, cfg.uiDiagPin));	
    RaiseIRQ(DIAG_OUT,0);

}

