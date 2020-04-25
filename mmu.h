// mmu.h

// A Missing-materials-unit for MK404

#ifndef __MMU_H___
#define __MMU_H___

#include <stdbool.h>
#include <sim_avr.h>
#include <pthread.h>
#include <uart_pty.h>

typedef struct mmu_t{
    avr_t *avr;
    bool bQuit;
    bool bStarted;
    pthread_t run;
    uart_pty_t UART0;
    uart_pty_t *UARTHost;
} mmu_t;

mmu_t* mmu_init(avr_t *host, avr_irq_t *hostTX, avr_irq_t *hostRX, avr_irq_t *irqReset);

void mmu_start(mmu_t* p);

void mmu_stop(mmu_t* p);

#endif