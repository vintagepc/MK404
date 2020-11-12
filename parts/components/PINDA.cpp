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
#include "TelemetryHost.h"
#include "gsl-lite.hpp"
#include <cmath>    // for pow, floor, round, sqrt
#include <cstring>
#include <iostream>

//#define TRACE(_w)_w
#ifndef TRACE
#define TRACE(_w)
#endif

// PINDA image definition:
#ifdef ENABLE_PINDA_IMAGE
static constexpr char PINDA_IMAGE[] =
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000050c00000000000000000000000000000000"
"0000000000000000000000000a242f2e200b0000000000000000000000000000"
"000000000000000000000001133549514b391800000000000000000000000000"
"0000000000000000000000163f58646254391200000000000000000000000000"
"0000000000000000000000072d4e646b66543407000000000000000000000000"
"0000000000000000000000163f58646254391200000000000000000000000000"
"000000000000000000000001133549514b391800000000000000000000000000"
"0000000000000000000000000a242f2e200b0000000000000000000000000000"
"0000000000000000000000000000050c00000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000"
"0000000000000000000000000000000000000000000000000000000000000000";

void PINDA::ParseStringToMap()
{
	Expects(sizeof(PINDA_IMAGE)==2049); //32x32 with trailing null.
	const gsl::span<const char> cImg { PINDA_IMAGE };
	for (size_t i=0; i<cImg.size_bytes()-1; i+=2)
	{
		char strByte[3] = {cImg.at(i), cImg.at(i+1), '\0'};
		m_uiScan[i/2] = gsl::narrow<uint8_t>(std::stoul(static_cast<char*>(strByte),0,16));
	};
	// std::cout << "Parsed image:\n";
	// for (size_t i=0; i<1024; i++)
	// {
	// 	std::cout << std::width(2) << std::hex << (uint)m_uiScan.at(i);
	// 	if ((i+1)%32==0) std::cout << '\n';

	// }

}

#endif

// This creates an inverted parabolic trigger zone above the cal point
void PINDA::CheckTriggerNoSheet()
{
    float fEdist = 100;
    bool bFound = false;
    //printf("PINDA: X: %f Y: %f\n", m_fPos[0], m_fPos[1]);
	float fdX = 0, fdY = 0;

	if (m_fPos[2]<10.f)
	{
		for (auto i=0U; i<GetXYCalPoints().size()/2; i++)
		{
			fdX = m_fPos[0] - GetXYCalPoints().at(2*i);
			fdY = m_fPos[1] - GetXYCalPoints().at((2*i)+1);
			fEdist = sqrt( pow(fdX,2) + pow(fdY ,2));
			if (fEdist<21)
			{
				bFound = true;
				break;  // Stop as soon as we find a near cal point.
			}
		}
	}
    // Now calc z trigger height for the given distance from the point center
    if (bFound)
    {
#ifdef ENABLE_PINDA_IMAGE
		// The firmware runs the scan grid 1024 steps to either side of the center.
		float fTrigZ = 0.f;
		// There's a search area of 2048 motor steps  (+/- 1024 on each side of the point center)
		// at 100 steps/mm, hence the 20.48.
		// We are breaking this up into 32 "pixels", so each (32/20.48) mm = one index in the "image" array.
		// fdX/Y are offsets from the point center, so should range from -<something> to +<something>; this needs to map to 0->31.
		// Ew, the +9 on Y is weird, that should be +15 or +16....
		float fXIdx = 15.5f + ((fdX)*32.f)/20.48f, fYIdx = 9.f + ((fdY)*32.f)/20.48f;
		int iX = fXIdx, iY = fYIdx;
		float fDiff = fXIdx - static_cast<float>(iX);

		if ((iX>=0 && iX<32) && (iY>=0 && iY<32))
		{
			float fZByte = 0;
			if (iX==31)
				fZByte = m_uiScan.at((32*iY)+iX);
			else
			{ // Lerp the neighbouring values if inbetween.
				fZByte = (fDiff*m_uiScan.at((32*iY)+iX+1)) + ((1.f-fDiff)*m_uiScan.at((32*iY)+iX));
			}
			// 3.468 is a magic number of some kind determined by trial/error; I suspect the correct value
			// is obvious to someone more familiar with the search algorithm.
			if (fZByte>0)
				fTrigZ = 3.47+(fZByte/400.f);  // Z scaling comes from 255 z steps being the range. 3.468
		}
#else
		bool bHasSheet = m_XYCalType != XYCalMap::MK2;
		float fTrigZ = + (bHasSheet? 3.0 : 0.0);
        fTrigZ += (1.0*(1-pow(fEdist/5,2)));
#endif
        //printf("fTZ:%f fZ: %f\n",fTrigZ, this->fPos[2]);
        if (m_fPos[2]<=fTrigZ)
		{
            RaiseIRQ(TRIGGER_OUT,1);
		}
        else
		{
            RaiseIRQ(TRIGGER_OUT,0);
		}
    }
	else
	{
		RaiseIRQ(TRIGGER_OUT,0);
	}
}

Scriptable::LineStatus PINDA::ProcessAction (unsigned int iAct, const std::vector<std::string> &vArgs)
{
	switch (iAct)
	{
		case ActToggleSheet:
			ToggleSheet();
			return LineStatus::Finished;
		case ActSetPos:
		{
			uint32_t uiVal;
			float fIn = std::stof(vArgs.at(0));
			std::memcpy(&uiVal, &fIn,4);
			RaiseIRQ(X_POS_IN, uiVal);
			fIn = std::stof(vArgs.at(1));
			std::memcpy(&uiVal, &fIn,4);
			RaiseIRQ(Y_POS_IN, uiVal);
			fIn = std::stof(vArgs.at(2));
			std::memcpy(&uiVal, &fIn,4);
			RaiseIRQ(Z_POS_IN, uiVal);
			return LineStatus::Finished;
		}
		case ActSetMBLPoint:
		{
			int iVal = stoi(vArgs.at(0));
			if (iVal<0 || iVal > 48)
			{
				return IssueLineError(std::string("Index ") + std::to_string(iVal) + " is out of range [0,48]");
			}
			float fVal = stof(vArgs.at(1));
			gsl::at(m_mesh.points,iVal) = fVal;
			return LineStatus::Finished;
		}
		case ActSetXYCalPont:
		{
			int iVal = stoi(vArgs.at(0));
			if ((iVal<0) | (iVal>3))
			{
				return IssueLineError(std::string("Index ") + std::to_string(iVal) + " is out of range [0,3]");
			}
			float fX = stof(vArgs.at(1)), fY = stof(vArgs.at(2));
			GetXYCalPoints().at(2*iVal) = fX;
			GetXYCalPoints().at((2*iVal)+1) = fY;
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
    uint8_t iX = floor(((m_fPos[0] - m_fOffset[0])/255.0)*7);
    uint8_t iY = floor(((m_fPos[1] - m_fOffset[1])/210.0)*7);

    float fZTrig = gsl::at(m_mesh.points,iX+(7*iY));

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

void PINDA::OnXChanged(struct avr_irq_t*,uint32_t value)
{
    float fVal;
	std::memcpy(&fVal,&value,4);
     m_fPos[0] = fVal + m_fOffset[0];
    // We only need to check triggering on XY motion for selfcal
    if (!m_bIsSheetPresent) CheckTriggerNoSheet();

}

void PINDA::OnYChanged(struct avr_irq_t*,uint32_t value)
{
    float fVal;
	std::memcpy(&fVal,&value,4);
     m_fPos[1] = fVal + m_fOffset[1];
    // We only need to check triggering on XY motion for selfcal
    if (!m_bIsSheetPresent) CheckTriggerNoSheet();

}

gsl::span<float>& PINDA::GetXYCalPoints()
{
    // pulled from mesh_bed_calibration.cpp
    static float _fMK3Cal[8] = {
        37.f -2.0, 18.4f -9.4 + 2,
        245.f -2.0, 18.4f - 9.4 + 2,
        245.f -2.0, 210.4f - 9.4 + 2,
        37.f -2.0,  210.4f -9.4 + 2
    };
	static float _fMK25Cal[8] = {
        37.f -2.0, 18.4f -9.4 + 2,
        245.f -2.0, 18.4f - 9.4 + 2,
        245.f -2.0, 210.4f - 9.4,
        37.f -2.0,  210.4f -9.4
    };
	static float _fMK2Cal[18] = {
		 13.f + 22.f,	  6.4f + 3,
		 13.f + 22.f, 	104.4f + 3,
		 13.f + 22.f, 	202.4f + 3,
		115.f + 22.f, 	  6.4f + 3,
		115.f + 22.f, 	104.4f + 3,
		115.f + 22.f,	202.4f + 3,
		216.f + 22.f, 	  6.4f + 3,
		216.f + 22.f, 	104.4f + 3,
		216.f + 22.f, 	202.4f + 3
	};

	static gsl::span<float> fMK3 {_fMK3Cal};
	static gsl::span<float> fMK25 {_fMK25Cal};
	static gsl::span<float> fMK2 {_fMK2Cal};

	switch (m_XYCalType)
	{
		case XYCalMap::MK2:
			return fMK2;
		case XYCalMap::MK25:
			return fMK25;
		case XYCalMap::MK3:
		default:
			return fMK3;
	}


}

void PINDA::OnZChanged(avr_irq_t*, uint32_t value)
{
    // Z is translated so that the bed level heights don't need to account for it, e.g. they are just
    // zero-referenced against this internal "z" value.
    float fVal;
	std::memcpy(&fVal,&value,4);
    m_fPos[2] = fVal - m_fZTrigHeight;
    if (!m_bIsSheetPresent)
	{
        CheckTriggerNoSheet();
	}
    else
	{
        CheckTrigger();
	}
}


void PINDA::SetMBLMap()
{
    // Eventually. read this from a file or so. For now just set it explicitly:
    // Double braces are to squelch GCC bug 53119
    //m_mesh = ;
}

void PINDA::ToggleSheet()
{
	if (m_XYCalType == XYCalMap::MK2) return;
    m_bIsSheetPresent=!m_bIsSheetPresent;
    std::cout << "Steel sheet: " << (m_bIsSheetPresent? "INSTALLED\n" : "REMOVED\n");
    RaiseIRQ(SHEET_OUT,m_bIsSheetPresent);
}

PINDA::PINDA(float fX, float fY, XYCalMap map):Scriptable("PINDA"),m_fOffset{fX,fY}, m_XYCalType(map)
{
	m_bIsSheetPresent = false; //m_XYCalType != XYCalMap::MK2;
    SetMBLMap();
	RegisterKeyHandler('y', "Toggles the steel sheet on the heatbed");
#ifdef ENABLE_PINDA_IMAGE
	ParseStringToMap();
#endif

}

void PINDA::OnKeyPress(const Key &key)
{
	switch (key)
	{
		case 'y':
			ToggleSheet();
			break;
	}
}

void PINDA::Reconfigure(float fX, float fY, XYCalMap map)
{
	m_XYCalType = map;
	m_fOffset[0] = fX;
	m_fOffset[1] = fY;
	m_bIsSheetPresent = m_XYCalType != XYCalMap::MK2;

}

void PINDA::Init(struct avr_t * avr, avr_irq_t *irqX, avr_irq_t *irqY, avr_irq_t *irqZ)
{
    _Init(avr, this);

	// The MK2 does not have a separate set of MBL points or a removable sheet.
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
    //RaiseIRQ(TRIGGER_OUT,0);

	auto &TH = TelemetryHost::GetHost();
	TH.AddTrace(this, TRIGGER_OUT, {TC::InputPin, TC::Misc});
	TH.AddTrace(this, SHEET_OUT, {TC::InputPin, TC::Misc});

}
