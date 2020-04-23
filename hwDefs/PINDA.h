/*
    PINDA sim for Einsy Rambo

*/

#ifndef __PINDA_H__
#define __PINDA_H__

#include "sim_irq.h"
#include "stdbool.h"

enum {
	IRQ_PINDA_X_POS_IN = 0,
    IRQ_PINDA_Y_POS_IN,
    IRQ_PINDA_Z_POS_IN,
    IRQ_PINDA_TRIGGER_OUT,
	IRQ_PINDA_COUNT
};

// so we can use initializer syntax later
typedef struct 
{   
    float points[49];
} MBLMap_t;


typedef struct pinda_t {
	avr_irq_t * irq;	// output irq
	struct avr_t * avr;
	float fZTrigHeight; // Trigger height above Z=0, i.e. the "zip tie" adjustment
    float fOffset[2]; // pinda X Y offset  from nozzle
    float fPos[3]; // Current position tracking.
    MBLMap_t mesh;// MBL map 
    bool bIsSheetPresent; // Is the steel sheet present? IF yes, PINDA will attempt to simulate the bed sensing point for selfcal instead.
} pinda_t;


void
pinda_init(
		struct avr_t * avr,
		pinda_t *p,
        float fX,
        float fY,
        avr_irq_t *irqX,
        avr_irq_t *irqY,
        avr_irq_t *irqZ);

#endif /* __PINDA_H__*/
