
#include "mmu.h"
#include "sim_avr.h"
#include <stdio.h>
#include "sim_hex.h"
#include <stdlib.h>
#include <string.h>
#include "avr_extint.h"
#include "avr_uart.h"
#include "avr_ioport.h"

struct mmu_t mmu;

static void *
avr_run_thread(
		void * ignore)
{
	printf("Starting MMU2 execution...\n");
	int state = cpu_Running;
	while ((state != cpu_Done) && (state != cpu_Crashed) && !mmu.bQuit){	
		//if (gbPrintPC)
		//	printf("PC: %x\n",mmu->pc);
		state = avr_run(mmu.avr);
	}
	avr_terminate(mmu.avr);
	printf("MMU finished.\n");
	return NULL;
}

static void start_mmu_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{

	printf("MMU RESET: %02x\n",value);
    mmu_t *pMMU = (mmu_t*) param;
	if (value==0)
		mmu_start(pMMU);
    else
        printf("MMU RESET (TODO)\n");
}

static void mmu_tx_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
    //printf("MMU said: %02x\n",value);
    avr_irq_t *pHostRX = (avr_irq_t*) param;
    avr_raise_irq(pHostRX,value);
}

static void mmu_host_tx_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
    //printf("Host said: %02x\n",value);
    mmu_t *pMMU = (mmu_t*) param;
    avr_raise_irq(avr_io_getirq(pMMU->avr, AVR_IOCTL_UART_GETIRQ('1'), UART_IRQ_INPUT),value);
}

mmu_t* mmu_init(avr_t *host, avr_irq_t *hostTX, avr_irq_t *hostRX,  avr_irq_t *irqReset)
{
	uint32_t boot_base, boot_size;
	char * mmcu = "atmega32u4";
	uint32_t freq = 16000000;

	avr_t *avr = avr_make_mcu_by_name(mmcu);
    mmu.bQuit = false;
    mmu.bStarted = false;
    mmu.avr = avr;

	if (!avr) {
		fprintf(stderr, "Error creating the MMU2 core\n");
		return NULL;
	}
    char boot_path[] = "MM-control-01.hex";
	 uint8_t * boot = read_ihex_file(boot_path, &boot_size, &boot_base);
	 if (!boot) {
		fprintf(stderr, "MMU2: Unable to load %s\n", boot_path);
		return NULL;
	 }
	 printf("%s f/w 0x%05x: %d bytes\n", mmcu, boot_base, boot_size);
	avr_init(avr);

	avr->frequency = freq;
	avr->vcc = 5000;
	avr->aref = 0;
	avr->avcc = 5000;
	memcpy(avr->flash + boot_base, boot, boot_size);
	printf("fw base at:%u\n",boot_base);
	free(boot);
    /* end of flash, remember we are writing /code/ */
	avr->codeend = avr->flashend;
	avr->log = 1;

	// even if not setup at startup, activate gdb if crashing
	avr->gdb_port = 1234;
	
	// suppress continuous polling for low INT lines... major performance drain.
	for (int i=0; i<8; i++)
		avr_extint_set_strict_lvl_trig(avr,i,false);

    uart_pty_init(avr, &mmu.UART0);

	avr_irq_register_notify(irqReset, start_mmu_hook, &mmu);
    //avr_irq_register_notify(hostTX, mmu_host_tx_hook, &mmu);

    //avr_irq_register_notify(avr_io_getirq(mmu.avr, AVR_IOCTL_UART_GETIRQ('1'), UART_IRQ_OUTPUT), mmu_tx_hook, hostRX);


    printf("MMU UART:\n");
    uart_pty_connect(&mmu.UART0,'1');

    return &mmu;
}
void mmu_start(mmu_t *this)
{
    if (this->bStarted)
        return; 
    printf("Starting MMU...\n");
    this->bStarted = true;
	pthread_create(&mmu.run, NULL, avr_run_thread, NULL);
}

void mmu_stop(mmu_t *this){
    uart_pty_stop(&mmu.UART0);
    if (!this->bStarted)
        return;
    this->bQuit = true;
    pthread_join(this->run,NULL);
    
    printf("MMU Done\n");
}

