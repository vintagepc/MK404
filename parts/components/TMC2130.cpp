/*
	TMC2130.cpp - a trinamic driver simulator for Einsy.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK3SIM.

	MK3SIM is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK3SIM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK3SIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TMC2130.h"
#include <GL/freeglut_std.h>  // for glutStrokeCharacter, GLUT_STROKE_MONO_R...
#include <GL/gl.h>            // for glVertex3f, glColor3f, glBegin, glEnd
#include <stdio.h>            // for printf
#include <string.h>           // for memset
#include <algorithm>          // for min
#include "TelemetryHost.h"

//#define TRACE(_w) _w
#define TRACE2(_w) if (m_cAxis=='S' || m_cAxis=='I') _w
#ifndef TRACE
#define TRACE(_w)
#endif


void TMC2130::Draw()
{
        if (!m_bConfigured)
            return; // Motors not ready yet.
        glColor3f(0,0,0);
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
            glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,m_cAxis);
        glPopMatrix();
        glColor3f(1,1,1);
        glPushMatrix();
            glTranslatef(280,7,0);
            glScalef(0.09,-0.05,0);
            string strPos = to_string(m_fCurPos);
            for (int i=0; i<min(7,(int)strPos.size()); i++)
                glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,strPos[i]);

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
				glVertex3f(m_fEnd,2,0);
				glVertex3f(m_fEnd+2,2,0);
				glVertex3f(m_fEnd+2,8,0);
				glVertex3f(m_fEnd,8,0);
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
        if (!m_bConfigured)
            return; // Motors not ready yet.
        glColor3f(0,0,0);
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
            glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,m_cAxis);
        glPopMatrix();
        glColor3f(1,1,1);
        glPushMatrix();
            glTranslatef(30,7,0);
            glScalef(0.09,-0.05,0);
			string strPos = to_string(m_fCurPos);
            for (int i=0; i<min(7,(int)strPos.size()); i++)
                glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,strPos[i]);
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
    TRACE(printf("tmc2130 %c cmd: w: %x a: %02x  d: %08x\n",m_cAxis, m_cmdProc.bitsIn.RW, m_cmdProc.bitsIn.address, m_cmdProc.bitsIn.data));
    if (m_cmdProc.bitsIn.RW)
    {
        m_regs.raw[m_cmdProc.bitsIn.address] = m_cmdProc.bitsIn.data;
        //printf("REG %c %02x set to: %010x\n", m_cAxis, m_cmdIn.bitsIn.address, m_cmdIn.bitsIn.data);

		if(m_cmdProc.bitsIn.address == 0x6C) // CHOPCONF
		{
			// updating CHOPCONF requires updating the current limits
			cfg.fStartPos = m_fCurPos;
			SetConfig(cfg);
		}
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
    TRACE(printf("TMC2130 %c: byte received: %02x (%010lx)\n",m_cAxis,value, m_cmdIn.all));
    // Clock out a reply byte, MSB first
    uint8_t byte = m_cmdOut.bytes[4];
    m_cmdOut.all<<=8;
    TRACE(printf("TMC2130 %c: Clocking (%10lx) out %02x\n",m_cAxis,m_cmdOut.all,byte));
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
	TRACE(printf("TMC2130 %c: CSEL changed to %02x\n",m_cAxis,value));
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
    TRACE(printf("TMC2130 %c: DIR changed to %02x\n",m_cAxis,value));
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
    if (!m_bEnable) return;
	if (!m_regs.defs.CHOPCONF.dedge)
	{
		// In normal mode only step on rising pulse
		if (!value || irq->value) return;
	}
	else
	{
		// With DEDGE step on each value change
		if (value == irq->value) return;
	}
    CancelTimer(m_fcnStandstill,this);
	//TRACE2(printf("TMC2130 %c: STEP changed to %02x\n",m_cAxis,value));
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
        else if (m_iCurStep>m_iMaxPos)
        {
            m_iCurStep = m_iMaxPos;
            bStall = true;
        }
    }

    m_fCurPos = StepToPos(m_iCurStep);
    uint32_t* posOut = (uint32_t*)(&m_fCurPos); // both 32 bits, just mangle it for sending over the wire.
    RaiseIRQ(POSITION_OUT, posOut[0]);
    TRACE(printf("cur pos: %f (%u)\n",m_fCurPos,m_iCurStep));
	bStall |= m_bStall;
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
	TRACE2(printf("TMC2130 %c: EN changed to %02x\n",m_cAxis.load(),value));
    m_bEnable = value==0; // active low, i.e motors off when high.
}

TMC2130::TMC2130(char cAxis):Scriptable(string("") + cAxis),m_cAxis(cAxis)
{
    memset(&m_regs.raw, 0, sizeof(m_regs.raw));
    m_regs.defs.DRV_STATUS.stst = true;
    m_regs.defs.DRV_STATUS.SG_RESULT = 250;
    m_regs.defs.GSTAT.reset = 1; // signal reset
	RegisterActionAndMenu("ToggleStall","Toggles the stallguard condition on the next step.",ActToggleStall);
	RegisterActionAndMenu("Stall","Sets the diag flag immediately.",ActSetDiag);
	RegisterActionAndMenu("Reset","Clears the diag flag immediately",ActResetDiag);
}

Scriptable::LineStatus TMC2130::ProcessAction (unsigned int iAct, const vector<string> &vArgs)
{
	switch (iAct)
	{
		case ActToggleStall:
			m_bStall^=true;
			return LineStatus::Finished;
		case ActSetDiag:
			RaiseIRQ(DIAG_OUT,1);
			return LineStatus::Finished;
		case ActResetDiag:
			RaiseIRQ(DIAG_OUT,0);
			return LineStatus::Finished;
	}
	return LineStatus::Unhandled;
}

void TMC2130::SetConfig(TMC2130_cfg_t cfgIn)
{
    cfg = cfgIn;
    m_iCurStep = PosToStep(cfg.fStartPos);
    m_iMaxPos = PosToStep(cfg.iMaxMM);
    m_fCurPos = cfg.fStartPos;
	m_fEnd = StepToPos(m_iMaxPos);
	m_bConfigured = true;
}

void TMC2130::Init(struct avr_t * avr)
{
    _Init(avr, this);

    RegisterNotify(DIR_IN,      MAKE_C_CALLBACK(TMC2130,OnDirIn), this);
    RegisterNotify(STEP_IN,     MAKE_C_CALLBACK(TMC2130,OnStepIn), this);
    RegisterNotify(ENABLE_IN,   MAKE_C_CALLBACK(TMC2130,OnEnableIn), this);

	auto pTH = TelemetryHost::GetHost();
	pTH->AddTrace(this, SPI_BYTE_IN,{TC::SPI, TC::Stepper},8);
	pTH->AddTrace(this, SPI_BYTE_OUT,{TC::SPI, TC::Stepper},8);
	pTH->AddTrace(this, SPI_CSEL, {TC::SPI, TC::Stepper, TC::OutputPin});
	pTH->AddTrace(this, STEP_IN,{TC::OutputPin, TC::Stepper});
	pTH->AddTrace(this, DIR_IN,{TC::OutputPin, TC::Stepper});
	pTH->AddTrace(this, ENABLE_IN,{TC::OutputPin, TC::Stepper});
	pTH->AddTrace(this, DIAG_OUT,{TC::InputPin, TC::Stepper});
}

float TMC2130::StepToPos(int32_t step)
{
	return (float)step/16*(float)(1u<<m_regs.defs.CHOPCONF.mres)/(float)cfg.uiStepsPerMM;
}

int32_t TMC2130::PosToStep(float pos)
{
	return pos*16/(float)(1u<<m_regs.defs.CHOPCONF.mres)*(float)cfg.uiStepsPerMM;
}
