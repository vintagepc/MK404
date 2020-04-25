/*
    PINDA sim for Einsy Rambo

*/
#include "PINDA.h"
#include "avr_ioport.h"
#include "math.h"

//#define TRACE(_w)_w
#ifndef TRACE
#define TRACE(_w)
#endif



// pulled from mesh_bed_calibration.cpp
static const float _bed_calibration_points[8] = {
    37.f -2.0, 18.4f -9.4 + 2,
    245.f -2.0, 18.4f - 9.4 + 2,
    245.f -2.0, 210.4f - 9.4 + 2,
	37.f -2.0,  210.4f -9.4 + 2
};

static const char * _pinda_irq_names[IRQ_PINDA_COUNT] = {
        [IRQ_PINDA_X_POS_IN] = "<pinda.x",
        [IRQ_PINDA_Y_POS_IN] = "<pinda.y",
        [IRQ_PINDA_Z_POS_IN] = "<pinda.z",
        [IRQ_PINDA_TRIGGER_OUT] = ">pinda.trigger"
};

// This creates an inverted parabolic trigger zone above the cal point
static void pinda_check_trigger_calpoint(pinda_t * this)
{
    float fEdist = 100;
    bool bFound = false;
    //printf("PINDA: X: %f Y: %f\n", this->fPos[0], this->fPos[1]);
    for (int i=0; i<4; i++)
    {
        fEdist = sqrt( pow(this->fPos[0] - _bed_calibration_points[2*i],2)  + 
            pow(this->fPos[1] - _bed_calibration_points[(2*i)+1],2));
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
        if (this->fPos[2]<=fTrigZ)
            avr_raise_irq(this->irq + IRQ_PINDA_TRIGGER_OUT,1);
        else
            avr_raise_irq(this->irq + IRQ_PINDA_TRIGGER_OUT,0);
    }
}

// Checks the current XYZ position against the MBL map and does calculations. 
static void pinda_check_trigger(pinda_t * this)
{

    // Bail early if too high to matter, to avoid needing to do all the math. 
    if (this->fPos[2]>5)
        return;

    // Just calc the nearest MBL point and report it. 
    uint8_t iX = round(((this->fPos[0] - this->fOffset[0])/255.0)*7);
    uint8_t iY = floor(((this->fPos[1] - this->fOffset[1])/210.0)*7);

    float fZTrig = this->mesh.points[iX+(7*iY)];
    
    if (this->fPos[2]<=fZTrig)
    {
        //printf("Trig @ %u %u\n",iX,iY);
        avr_raise_irq(this->irq + IRQ_PINDA_TRIGGER_OUT,1);
    }
    else if (this->fPos[2]<=fZTrig + 0.5) // Just reset to 0 in a small distance above the trigger, to avoid IRQspam.
        avr_raise_irq(this->irq + IRQ_PINDA_TRIGGER_OUT,0);
}

static void pinda_x_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param )
{
    pinda_t *this = (pinda_t*)param;
    float *fVal = (float*) &value; // demangle the pos cast.
    this->fPos[0] = fVal[0] + this->fOffset[0];

    // We only need to check triggering on XY motion for selfcal
    if (!this->bIsSheetPresent)
        pinda_check_trigger_calpoint(this); 

}

static void pinda_y_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param )
{
    pinda_t *this = (pinda_t*)param;
    float *fVal = (float*) &value; // demangle the pos cast.
    this->fPos[1] = fVal[0] + this->fOffset[1];

    // We only need to check triggering on XY motion on selfcal
    if (!this->bIsSheetPresent)
        pinda_check_trigger_calpoint(this); 

}

static void pinda_z_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param )
{
    pinda_t *this = (pinda_t*)param;
    // Z is translated so that the bed level heights don't need to account for it, e.g. they are just
    // zero-referenced against this internal "z" value.
    float *fVal = (float*) &value; // demangle the pos cast.
    this->fPos[2] = fVal[0] - this->fZTrigHeight; 
    if (!this->bIsSheetPresent)
        pinda_check_trigger_calpoint(this); 
    else
        pinda_check_trigger(this);
}


void pinda_setMBL_map(pinda_t *this)
{
    // TODO: read this from a file or so. For now just set it explicitly:
    // Double braces are to squelch GCC bug 53119
    this->mesh = (MBLMap_t){{0.04584,	0.07806,	0.10584,	0.12917,	0.14806,	0.1625, 	0.1725,
        0.00973,	0.04031,	0.06306,	0.07797,	0.08503,	0.08426,	0.07565,
        -0.02055,	0.00834,	0.02682,	0.03491,	0.0326,	    0.01988,	-0.00324,
        -0.045,	    -0.01787,	-0.00287,	0,	        -0.00926,	-0.03064,	-0.06416,
        -0.06361,	-0.0383,	-0.02602,	-0.02676,	-0.04052,	-0.06731,	-0.10713,
        -0.07639,	-0.05296,	-0.04262,	-0.04537,	-0.0612,	-0.09012,	-0.13213,
        -0.08333,	-0.06185,	-0.05268,	-0.05583,	-0.07129,	-0.09907,	-0.13916}};
}

void
pinda_init(
		struct avr_t * avr,
		pinda_t *this,
         float fX,
        float fY,
        avr_irq_t *irqX,
        avr_irq_t *irqY,
        avr_irq_t *irqZ)
{
    this->avr = avr;
    this->fOffset[0] = fX;
    this->fOffset[1] = fY;
    this->fZTrigHeight = 1.0f;
    this->bIsSheetPresent = true;

    for (int i=0; i<2; i++)
        this->fPos [i] = 0.0f; 

    pinda_setMBL_map(this);

    this->bIsSheetPresent = true; // Sheet is here by default.
    this->irq = avr_alloc_irq(&avr->irq_pool,0,IRQ_PINDA_COUNT,_pinda_irq_names);
    avr_connect_irq(irqX, this->irq + IRQ_PINDA_X_POS_IN);
    avr_connect_irq(irqY, this->irq + IRQ_PINDA_Y_POS_IN);
    avr_connect_irq(irqZ, this->irq + IRQ_PINDA_Z_POS_IN);

    avr_irq_register_notify(this->irq + IRQ_PINDA_X_POS_IN, pinda_x_hook,this);
    avr_irq_register_notify(this->irq + IRQ_PINDA_Y_POS_IN, pinda_y_hook,this);
    avr_irq_register_notify(this->irq + IRQ_PINDA_Z_POS_IN, pinda_z_hook,this);
    avr_raise_irq(this->irq + IRQ_PINDA_TRIGGER_OUT,0);
}