// mmu.h

// A Missing-materials-unit for MK404

#ifndef __MMU_H___
#define __MMU_H___

#include <stdbool.h>
#include <sim_avr.h>
#include <pthread.h>
#include <uart_pty.h>
#include "hc595.h"
#include "mmu_buttons.h"
#include "led.h"
#include "TMC2130.h"

typedef struct mmu_t{
    avr_t *avr;
    bool bQuit;
    bool bStarted;
    bool bReset;
    pthread_t run;
    uart_pty_t UART0;
    uart_pty_t *UARTHost;
    hc595_t shift;
    tmc2130_t Sel, Idl, Extr;
    led_t lGreen[5], lRed[5];
    buttons_t buttons;
} mmu_t;

typedef union hc595_data_t{
    uint32_t raw;
    struct {
        uint8_t b0 :1;
        uint8_t b1 :1;
        uint8_t b2 :1;
        uint8_t b3 :1;
        uint8_t b4 :1;
        uint8_t b5 :1;
        uint8_t b6 :1;
        uint8_t b7 :1;
        uint8_t b8 :1;
        uint8_t b9 :1;
        uint8_t b10 :1;
        uint8_t b11 :1;
        uint8_t b12 :1;
        uint8_t b13 :1;
        uint8_t b14 :1;
        uint8_t b15 :1;
    }bits;
}hc595_data_t;

mmu_t* mmu_init(avr_t *host, avr_irq_t *hostTX, avr_irq_t *hostRX, avr_irq_t *irqReset);

void mmu_start(mmu_t* p);

void mmu_startGL(mmu_t* p);

//void displayMMU(mmu_t *p);

void mmu_stop(mmu_t* p);

#endif