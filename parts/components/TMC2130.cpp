/*
	TMC2130.cpp - a trinamic driver simulator for Einsy.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK404.

	MK404 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK404 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK404.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TMC2130.h"
#include "TelemetryHost.h"
#include "gsl-lite.hpp"

#include <GL/freeglut_std.h>          // for glutStrokeCharacter, GLUT_STROKE_MONO_R...
#if defined(__APPLE__)
# include <OpenGL/gl.h>       // for glVertex3f, glColor3f, glBegin, glEnd
#else
# include <GL/gl.h>           // for glVertex3f, glColor3f, glBegin, glEnd
#endif
#include <algorithm>          // for min
#include <cmath>
#include <cstring>           // for memset

//#define TRACE(_w) _w
#define TRACE2(_w) if (m_cAxis=='S' || m_cAxis=='I') _w
#ifndef TRACE
#define TRACE(_w)
#endif

void TMC2130::Draw()
{
	_Draw(false);
}

void TMC2130::Draw_Simple()
{
	_Draw(true);
}

void TMC2130::_Draw(bool bIsSimple)
{
        if (!m_bConfigured)
		{
            return; // Motors not ready yet.
		}
		// Copy atomic to local
		float fPos = m_fCurPos;
        glColor3f(0,0,0);
	    glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(350,0,0);
			glVertex3f(350,10,0);
			glVertex3f(0,10,0);
			glColor3f(1,1,1);
			if (m_bEnable)
			{
				glVertex3f(3,8,0);
				glVertex3f(13,8,0);
				glVertex3f(13,1,0);
				glVertex3f(3,1,0);
				glColor3f(0,0,0);
			}
		glEnd();
        glPushMatrix();
            glTranslatef(3,7,0);
            glScalef(0.09,-0.05,0);
            glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,m_cAxis);
            //glTranslatef(  bIsSimple? 30 : 280 ,7,0);
            //glScalef(0.09,-0.05,0);
			// Values translated according to existing Scalef()
			glTranslatef(bIsSimple? 195 : 2973 ,0,0);
			glColor3f(1,1,1);
            std::string strPos = std::to_string(fPos);
            for (int i=0; i<std::min(7,static_cast<int>(strPos.size())); i++)
			{
                glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,strPos[i]);
			}
        glPopMatrix();
		if (bIsSimple)
		{
			return;
		}
		glPushMatrix();
			glTranslatef(20,0,0);
			glColor3f(1,0,0);
			glBegin(GL_QUADS);
				glVertex3f(0,2,0);
				glVertex3f(-2,2,0);
				glVertex3f(-2,8,0);
				glVertex3f(0,8,0);
				glVertex3f(m_fEnd,2,0);
				glVertex3f(m_fEnd+2,2,0);
				glVertex3f(m_fEnd+2,8,0);
				glVertex3f(m_fEnd,8,0);
				glColor3f(0,1,1);
				glVertex3f(fPos-0.5,2,0);
				glVertex3f(fPos+0.5,2,0);
				glVertex3f(fPos+0.5,8,0);
				glVertex3f(fPos-0.5,8,0);
			glEnd();
		glPopMatrix();
}

void TMC2130::CreateReply()
{
    m_cmdOut.all = 0x00; // Copy over.
    if (m_cmdProc.bitsIn.RW == 0) // Last in was a read.
    {
        m_cmdOut.bitsOut.data = gsl::at(m_regs.raw,m_cmdProc.bitsIn.address);
        if (m_cmdProc.bitsIn.address == 0x01)
		{
            m_regs.raw[0x01] = 0; // GSTAT is cleared after read.
		}
        TRACE(printf("Reading out %02x (%10lx)\n", m_cmdProc.bitsIn.address, m_cmdOut.bitsOut.data));
    }
    else
	{
        m_cmdOut.bitsOut.data = m_cmdProc.bitsOut.data;
	}
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
    TRACE(printf("tmc2130 %c cmd: w: %x a: %02x  d: %08lx\n",m_cAxis.load(), m_cmdProc.bitsIn.RW, m_cmdProc.bitsIn.address, m_cmdProc.bitsIn.data));
    if (m_cmdProc.bitsIn.RW)
    {
        gsl::at(m_regs.raw,m_cmdProc.bitsIn.address) = m_cmdProc.bitsIn.data;
        //printf("REG %c %02x set to: %010x\n", m_cAxis, m_cmdIn.bitsIn.address, m_cmdIn.bitsIn.data);
		switch (m_cmdProc.bitsIn.address)
		{
			case 0x00: // GCONF
				RaiseDiag(m_regs.defs.DRV_STATUS.stallGuard); // Adjust DIAG out, it mayhave  been reconfigured.
				break;
			case 0x6C: // Chopconf
				m_uiStepIncrement = std::pow(2,m_regs.defs.CHOPCONF.mres);
				break;
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
uint8_t TMC2130::OnSPIIn(struct avr_irq_t *, uint32_t value)
{
    m_cmdIn.all<<=8; // Shift bits up
    m_cmdIn.bytes[0] = value;
    TRACE(printf("TMC2130 %c: byte received: %02x (%010lx)\n",m_cAxis.load(),value, m_cmdIn.all));
    // Clock out a reply byte, MSB first
    uint8_t byte = m_cmdOut.bytes[4];
    m_cmdOut.all<<=8;
    TRACE(printf("TMC2130 %c: Clocking (%10lx) out %02x\n",m_cAxis.load(),m_cmdOut.all,byte));
    SetSendReplyFlag();
    return byte; // SPIPeripheral takes care of the reply.
}


void TMC2130::RaiseDiag(uint8_t value)
{
	// TODO (anyone) - right now these are tied together because the Einsy board does so.
	// But in the future we may need to separate this out to toggle DIAG0 and DIAG one separately.
    bool bDiag = m_regs.defs.GCONF.diag0_stall || m_regs.defs.GCONF.diag1_stall;
    //printf("%c Diag: %01x SG %d PP %01x %01x \n",m_cAxis.load(), bDiag, m_regs.defs.DRV_STATUS.stallGuard, m_regs.defs.GCONF.diag0_int_pushpull, value );
    if (bDiag)
	{
		//printf("Raised: %d\n", value ==  m_regs.defs.GCONF.diag0_int_pushpull);
        RaiseIRQ(DIAG_OUT, value == m_regs.defs.GCONF.diag0_int_pushpull);
	}
}

// Called when CSEL changes.
void TMC2130::OnCSELIn(struct avr_irq_t *, uint32_t value)
{
	TRACE(printf("TMC2130 %c: CSEL changed to %02x\n",m_cAxis.load(),value));
    if (value == 1) // Just finished a CSEL
    {
        m_cmdProc = m_cmdIn;
        ProcessCommand();
    }
}

// Called when DIR pin changes.
void TMC2130::OnDirIn(struct avr_irq_t * , uint32_t value)
{
    TRACE(printf("TMC2130 %c: DIR changed to %02x\n",m_cAxis.load(),value));
    m_bDir = value^cfg.bInverted; // XOR
}

avr_cycle_count_t TMC2130::OnStandStillTimeout(avr_t *, avr_cycle_count_t)
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
	{
        m_iCurStep-=m_uiStepIncrement;
	}
    else
	{
        m_iCurStep+=m_uiStepIncrement;
	}
    bool bStall = false;
    if (!cfg.bHasNoEndStops)
    {
        if (m_iCurStep<0)
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
    uint32_t posOut;
	std::memcpy (&posOut, &m_fCurPos, 4);
    RaiseIRQ(POSITION_OUT, posOut);
	RaiseIRQ(STEP_POS_OUT, m_iCurStep);
    TRACE(printf("cur pos: %f (%u)\n",m_fCurPos,m_iCurStep));
	bStall |= m_bStall;
    if (bStall)
    {
        RaiseDiag(1);
        m_regs.defs.DRV_STATUS.SG_RESULT = 0;
    }
    else if (!bStall && m_regs.defs.DRV_STATUS.stallGuard)
    {
          RaiseDiag(0);
          m_regs.defs.DRV_STATUS.SG_RESULT = 250;
    }
    m_regs.defs.DRV_STATUS.stallGuard = bStall;
    m_regs.defs.DRV_STATUS.stst = false;
    // 2^20 comes from the datasheet.
    RegisterTimer(m_fcnStandstill,1U<<20U,this);
}

// Called when DRV_EN is triggered.
void TMC2130::OnEnableIn(struct avr_irq_t *, uint32_t value)
{
	TRACE(printf("TMC2130 %c: EN changed to %02x\n",m_cAxis.load(),value));
    m_bEnable = value==0; // active low, i.e motors off when high.
}

// needed because cppcheck doesn't seem to do bitfield unions correctly.
// cppcheck-suppress uninitMemberVar
TMC2130::TMC2130(char cAxis):Scriptable(std::string("") + cAxis),m_cAxis(cAxis)
{
		// Check register packing/sizes:
	Expects(sizeof(m_regs) == sizeof(m_regs.raw));
	Expects(
		sizeof(m_cmdIn.bitsIn)==sizeof(m_cmdIn.bytes) &&
		sizeof(m_cmdIn.bitsOut) == sizeof(m_cmdIn.bytes)
	);
    memset(&m_regs.raw, 0, sizeof(m_regs.raw));

    m_regs.defs.DRV_STATUS.stst = true;
    m_regs.defs.DRV_STATUS.SG_RESULT = 250;
    m_regs.defs.GSTAT.reset = 1; // signal reset
	RegisterActionAndMenu("ToggleStall","Toggles the stallguard condition on the next step.",ActToggleStall);
	RegisterActionAndMenu("Stall","Sets the diag flag immediately.",ActSetDiag);
	RegisterActionAndMenu("Reset","Clears the diag flag immediately",ActResetDiag);
}

Scriptable::LineStatus TMC2130::ProcessAction (unsigned int iAct, const std::vector<std::string> &)
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
    _InitWithArgs(avr, this, nullptr, SPI_CSEL);

    RegisterNotify(DIR_IN,      MAKE_C_CALLBACK(TMC2130,OnDirIn), this);
    RegisterNotify(STEP_IN,     MAKE_C_CALLBACK(TMC2130,OnStepIn), this);
    RegisterNotify(ENABLE_IN,   MAKE_C_CALLBACK(TMC2130,OnEnableIn), this);

	GetIRQ(DIR_IN)->flags |= IRQ_FLAG_FILTERED;
	GetIRQ(ENABLE_IN)->flags |= IRQ_FLAG_FILTERED;

	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, SPI_BYTE_IN,{TC::SPI, TC::Stepper},8);
	TH.AddTrace(this, SPI_BYTE_OUT,{TC::SPI, TC::Stepper},8);
	TH.AddTrace(this, SPI_CSEL, {TC::SPI, TC::Stepper, TC::OutputPin});
	TH.AddTrace(this, STEP_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, DIR_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, ENABLE_IN,{TC::OutputPin, TC::Stepper});
	TH.AddTrace(this, DIAG_OUT,{TC::InputPin, TC::Stepper});
}

float TMC2130::StepToPos(int32_t step)
{
	return static_cast<float>(step)/static_cast<float>(cfg.uiFullStepsPerMM);
}

int32_t TMC2130::PosToStep(float pos)
{
	return pos*static_cast<float>(cfg.uiFullStepsPerMM); // Convert pos to steps, we always work in the full 256 microstep workspace.
}
