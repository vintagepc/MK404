/*
	PINDA.h - a PINDA sim that can do MBL and axis skew calibrations.

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

#pragma once

#include <stdint.h>          // for uint32_t
#include <string>            // for string
#include <vector>            // for vector
#include "BasePeripheral.h"  // for BasePeripheral
#include "IScriptable.h"     // for IScriptable::LineStatus
#include "Scriptable.h"      // for Scriptable
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t
#include <atomic>

class PINDA:public BasePeripheral,public Scriptable{
    public:
        #define IRQPAIRS _IRQ(X_POS_IN,"<pinda.x_in") _IRQ(Y_POS_IN,"<pinda.y_in") _IRQ(Z_POS_IN,"<pinda.Z_in") _IRQ(TRIGGER_OUT,">pinda.out") _IRQ(SHEET_OUT,">sheet.out")
        #include "IRQHelper.h"

    // Creates a new PINDA with X/Y nozzle offsets fX and fY
    PINDA(float fX = 0, float fY = 0);

    // Initializes the PINDA on AVR, and connects it to the X/Y/Z position IRQs
    void Init(avr_t *avr, avr_irq_t *irqX, avr_irq_t *irqY, avr_irq_t *irqZ);

    // Toggles steel sheet presence. If it is removed, the PINDA will exhibit XY calibration trigger behaviour.
    void ToggleSheet();

	// so we can use initializer syntax later
	typedef struct
	{
		float points[49];
	} MBLMap_t;

	protected:
		LineStatus ProcessAction(unsigned int iAct, const vector<string> &vArgs);

private:

	enum Actions
	{
		ActToggleSheet,
		ActSetMBLPoint,
		ActSetXYCalPont
	};

    void OnXChanged(avr_irq_t *irq, uint32_t value);
    void OnYChanged(avr_irq_t *irq, uint32_t value);
    void OnZChanged(avr_irq_t *irq, uint32_t value);

    // Checks trigger z if no sheet is present.
    void CheckTriggerNoSheet();

    // Checks Z trigger if sheet is present (MBL)
    void CheckTrigger();

    void SetMBLMap();

	float m_fZTrigHeight = 1.0; // Trigger height above Z=0, i.e. the "zip tie" adjustment
    float m_fOffset[2] = {0,0}; // pinda X Y offset  from nozzle
    float m_fPos[3] = {10,10,10}; // Current position tracking.
    MBLMap_t m_mesh;// MBL map
    atomic_bool m_bIsSheetPresent {true}; // Is the steel sheet present? IF yes, PINDA will attempt to simulate the bed sensing point for selfcal instead.

    // pulled from mesh_bed_calibration.cpp
    float _bed_calibration_points[8] = {
        37.f -2.0, 18.4f -9.4 + 2,
        245.f -2.0, 18.4f - 9.4 + 2,
        245.f -2.0, 210.4f - 9.4 + 2,
        37.f -2.0,  210.4f -9.4 + 2
    };
};
