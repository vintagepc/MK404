/*
	PINDA.cpp - a PINDA sim that can do MBL and axis skew calibrations.

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

#include "PINDA.h"
#include <stdio.h>  // for printf
#include <cmath>    // for pow, floor, round, sqrt
#include <cstring>
#include "TelemetryHost.h"

//#define TRACE(_w)_w
#ifndef TRACE
#define TRACE(_w)
#endif

// This creates an inverted parabolic trigger zone above the cal point
void PINDA::CheckTriggerNoSheet()
{
    float fEdist = 100;
    bool bFound = false;
    //printf("PINDA: X: %f Y: %f\n", this->fPos[0], this->fPos[1]);
    for (int i=0; i<4; i++)
    {
        fEdist = sqrt( pow(m_fPos[0] - PINDA::_bed_calibration_points[2*i],2)  +
            pow(m_fPos[1] - PINDA::_bed_calibration_points[(2*i)+1],2));
        if (fEdist<10)
        {
            bFound = true;
            break;  // Stop as soon as we find a near cal point.
        }
    }
    // Now calc z trigger height for the given distance from the point center
    if (bFound)
    {
        float fTrigZ = (1.0*(1-pow(fEdist/5,2))) + 3.0 ;
        //printf("fTZ:%f fZ: %f\n",fTrigZ, this->fPos[2]);
        if (m_fPos[2]<=fTrigZ)
            RaiseIRQ(TRIGGER_OUT,1);
        else
            RaiseIRQ(TRIGGER_OUT,0);
    }
	else
	{
		RaiseIRQ(TRIGGER_OUT,0);
}
}

Scriptable::LineStatus PINDA::ProcessAction (unsigned int iAct, const vector<string> &vArgs)
{
	switch (iAct)
	{
		case ActToggleSheet:
			ToggleSheet();
			return LineStatus::Finished;
		case ActSetPos:
		{
			uint32_t uiVal;
			float fIn = stof(vArgs.at(0));
			std::memcpy(&uiVal, &fIn,4);
			RaiseIRQ(X_POS_IN, uiVal);
			fIn = stof(vArgs.at(1));
			std::memcpy(&uiVal, &fIn,4);
			RaiseIRQ(Y_POS_IN, uiVal);
			fIn = stof(vArgs.at(2));
			std::memcpy(&uiVal, &fIn,4);
			RaiseIRQ(Z_POS_IN, uiVal);
			return LineStatus::Finished;
		}
		case ActSetMBLPoint:
		{
			int iVal = stoi(vArgs.at(0));
			if (iVal<0 || iVal > 48)
				return IssueLineError(string("Index ") + to_string(iVal) + " is out of range [0,48]");
			float fVal = stof(vArgs.at(1));
			m_mesh.points[iVal] = fVal;
			return LineStatus::Finished;
		}
		case ActSetXYCalPont:
		{
			int iVal = stoi(vArgs.at(0));
			if ((iVal<0) | (iVal>3))
				return IssueLineError(string("Index ") + to_string(iVal) + " is out of range [0,3]");
			float fX = stof(vArgs.at(1)), fY = stof(vArgs.at(2));
			_bed_calibration_points[2*iVal] = fX;
			_bed_calibration_points[(2*iVal)+1] = fY;
			return LineStatus::Finished;
		}
	}
	return LineStatus::Unhandled;
}

// Checks the current XYZ position against the MBL map and does calculations.
void PINDA::CheckTrigger()
{
    // Bail early if too high to matter, to avoid needing to do all the math.
    if (m_fPos[2]>5)
	{
		RaiseIRQ(TRIGGER_OUT,0);
        return;
	}

    // Just calc the nearest MBL point and report it.
    uint8_t iX = round(((m_fPos[0] - m_fOffset[0])/255.0)*7);
    uint8_t iY = floor(((m_fPos[1] - m_fOffset[1])/210.0)*7);

    float fZTrig = m_mesh.points[iX+(7*iY)];

    if (m_fPos[2]<=fZTrig)
    {
        //printf("Trig @ %u %u\n",iX,iY);
        RaiseIRQ(TRIGGER_OUT,1);
    }
    else
	{
        RaiseIRQ(TRIGGER_OUT,0);
}
}

void PINDA::OnXChanged(struct avr_irq_t * irq,uint32_t value)
{
    float *fVal = (float*) &value; // demangle the pos cast.
     m_fPos[0] = fVal[0] + m_fOffset[0];
    // We only need to check triggering on XY motion for selfcal
    if (!m_bIsSheetPresent)
        CheckTriggerNoSheet();

}

void PINDA::OnYChanged(struct avr_irq_t * irq,uint32_t value)
{
    float *fVal = (float*) &value; // demangle the pos cast.
     m_fPos[1] = fVal[0] + m_fOffset[1];
    // We only need to check triggering on XY motion for selfcal
    if (!m_bIsSheetPresent)
        CheckTriggerNoSheet();

}

void PINDA::OnZChanged(avr_irq_t *irq, uint32_t value)
{
    // Z is translated so that the bed level heights don't need to account for it, e.g. they are just
    // zero-referenced against this internal "z" value.
    float *fVal = (float*) &value; // demangle the pos cast.
    m_fPos[2] = fVal[0] - m_fZTrigHeight;
    if (!m_bIsSheetPresent)
        CheckTriggerNoSheet();
    else
        CheckTrigger();
}


void PINDA::SetMBLMap()
{
    // TODO: read this from a file or so. For now just set it explicitly:
    // Double braces are to squelch GCC bug 53119
    m_mesh = (MBLMap_t){{0.04584,	0.07806,	0.10584,	0.12917,	0.14806,	0.1625, 	0.1725,
        0.00973,	0.04031,	0.06306,	0.07797,	0.08503,	0.08426,	0.07565,
        -0.02055,	0.00834,	0.02682,	0.03491,	0.0326,	    0.01988,	-0.00324,
        -0.045,	    -0.01787,	-0.00287,	0,	        -0.00926,	-0.03064,	-0.06416,
        -0.06361,	-0.0383,	-0.02602,	-0.02676,	-0.04052,	-0.06731,	-0.10713,
        -0.07639,	-0.05296,	-0.04262,	-0.04537,	-0.0612,	-0.09012,	-0.13213,
        -0.08333,	-0.06185,	-0.05268,	-0.05583,	-0.07129,	-0.09907,	-0.13916}};
}

void PINDA::ToggleSheet()
{
    m_bIsSheetPresent=!m_bIsSheetPresent;
    printf("Steel sheet: %s\n", m_bIsSheetPresent? "INSTALLED" : "REMOVED");
    RaiseIRQ(SHEET_OUT,m_bIsSheetPresent);
}

PINDA::PINDA(float fX, float fY):Scriptable("PINDA"),m_fOffset{fX,fY}
{
    SetMBLMap();
}

void PINDA::Init(struct avr_t * avr, avr_irq_t *irqX, avr_irq_t *irqY, avr_irq_t *irqZ)
{
    _Init(avr, this);

	RegisterActionAndMenu("ToggleSheet","Toggles the presence of the steel sheet",ActToggleSheet);
	RegisterAction("SetMBLPoint","Sets the given MBL point (0-48) to the given Z value",ActSetMBLPoint,{ArgType::Int,ArgType::Float});
	RegisterAction("SetXYPoint","Sets the (0-3)rd XY cal point position to x,y. (index, x,y)",ActSetXYCalPont,{ArgType::Int, ArgType::Float,ArgType::Float});
	RegisterAction("SetPos", "Sets X/Y/Z position of the probe", ActSetPos, {ArgType::Float, ArgType::Float, ArgType::Float});

    if (irqX) ConnectFrom(irqX, X_POS_IN);
    if (irqY) ConnectFrom(irqY, Y_POS_IN);
    if (irqZ) ConnectFrom(irqZ, Z_POS_IN);

    RegisterNotify(X_POS_IN, MAKE_C_CALLBACK(PINDA,OnXChanged),this);
    RegisterNotify(Y_POS_IN, MAKE_C_CALLBACK(PINDA,OnYChanged),this);
    RegisterNotify(Z_POS_IN, MAKE_C_CALLBACK(PINDA,OnZChanged),this);
	GetIRQ(TRIGGER_OUT)->flags |= IRQ_FLAG_FILTERED; // No retriggers.
    RaiseIRQ(TRIGGER_OUT,0);

	auto pTH = TelemetryHost::GetHost();
	pTH->AddTrace(this, TRIGGER_OUT, {TC::InputPin, TC::Misc});
	pTH->AddTrace(this, SHEET_OUT, {TC::InputPin, TC::Misc});

}
