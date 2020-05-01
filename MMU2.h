// mmu.h

// A Missing-materials-unit for MK404

#ifndef __MMU_H___
#define __MMU_H___

#include <pthread.h>
extern "C"
{
#include "uart_pty.h"
#include "mmu_buttons.h"
}
#include "HC595.h"
#include "LED.h"
#include "TMC2130.h"

#include "BasePeripheral.h"

class MMU2: public BasePeripheral
{

    public:
        #define IRQPAIRS _IRQ(FEED_DISTANCE,"<mmu.feed_distance") _IRQ(RESET,"<mmu.reset") _IRQ(PULLEY_IN,"<mmu.pulley_in")
        #include "IRQHelper.h"

        MMU2();

        void Init();

        void Start();

        void StartGL();

        void Stop();

        char* GetSerialPort();

    private:

        void* Run();

        void Draw();

        void InitGL();

        void OnDisplayTimer(int i);

        void OnResetIn(avr_irq_t *irq, uint32_t value);
        
        void OnPulleyFeedIn(avr_irq_t *irq, uint32_t value);

        void SetupHardware();

        bool m_bQuit = false;
        bool m_bStarted = false;
        bool m_bReset = false;
        pthread_t m_tRun;
        uart_pty_t m_UART0;
        HC595 m_shift;
        TMC2130 m_Sel, m_Idl, m_Extr;
        LED m_lGreen[5], m_lRed[5], m_lFINDA;
        buttons_t m_buttons;
        int m_iWindow = 0, m_iWinW = 0, m_iWinH = 0;

        static MMU2 *g_pMMU; // Needed for GL
};




#endif