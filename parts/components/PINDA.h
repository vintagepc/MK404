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

#include "BasePeripheral.h"  // for BasePeripheral
#include "IKeyClient.h"
#include "IScriptable.h"     // for IScriptable::LineStatus
#include "Scriptable.h"      // for Scriptable
#include "sim_avr.h"         // for avr_t
#include "sim_irq.h"         // for avr_irq_t
#include <atomic>
#include <cstdint>          // for uint32_t
#include <string>            // for string
#include <vector>            // for vector

class PINDA:public BasePeripheral,public Scriptable, private IKeyClient {
    public:
        #define IRQPAIRS _IRQ(X_POS_IN,"<pinda.x_in") _IRQ(Y_POS_IN,"<pinda.y_in") _IRQ(Z_POS_IN,"<pinda.Z_in") _IRQ(TRIGGER_OUT,">pinda.out") _IRQ(SHEET_OUT,">sheet.out")
        #include "IRQHelper.h"

    // Creates a new PINDA with X/Y nozzle offsets fX and fY
    explicit PINDA(float fX = 0, float fY = 0);

    // Initializes the PINDA on AVR, and connects it to the X/Y/Z position IRQs
    void Init(avr_t *avr, avr_irq_t *irqX, avr_irq_t *irqY, avr_irq_t *irqZ);

    // Toggles steel sheet presence. If it is removed, the PINDA will exhibit XY calibration trigger behaviour.
    void ToggleSheet();

	// so we can use initializer syntax later
	using MBLMap_t = struct
	{
		float points[49] {0.04584,	0.07806,	0.10584,	0.12917,	0.14806,	0.1625, 	0.1725,
        0.00973,	0.04031,	0.06306,	0.07797,	0.08503,	0.08426,	0.07565,
        -0.02055,	0.00834,	0.02682,	0.03491,	0.0326,	    0.01988,	-0.00324,
        -0.045,	    -0.01787,	-0.00287,	0,	        -0.00926,	-0.03064,	-0.06416,
        -0.06361,	-0.0383,	-0.02602,	-0.02676,	-0.04052,	-0.06731,	-0.10713,
        -0.07639,	-0.05296,	-0.04262,	-0.04537,	-0.0612,	-0.09012,	-0.13213,
        -0.08333,	-0.06185,	-0.05268,	-0.05583,	-0.07129,	-0.09907,	-0.13916};
	};

	protected:
		LineStatus ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs) override;

		void OnKeyPress(const Key &key) override;

private:

	enum Actions
	{
		ActToggleSheet,
		ActSetMBLPoint,
		ActSetXYCalPont,
		ActSetPos
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
    MBLMap_t m_mesh = MBLMap_t();// MBL map
    std::atomic_bool m_bIsSheetPresent {true}; // Is the steel sheet present? IF yes, PINDA will attempt to simulate the bed sensing point for selfcal instead.

    // pulled from mesh_bed_calibration.cpp
    float _bed_calibration_points[8] = {
        37.f -2.0, 18.4f -9.4 + 2,
        245.f -2.0, 18.4f - 9.4 + 2,
        245.f -2.0, 210.4f - 9.4 + 2,
        37.f -2.0,  210.4f -9.4 + 2
    };
};
