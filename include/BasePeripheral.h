#include <sim_irq.h>

class BasePeripheral 
{
    public:

        // Connects internal IRQ to an external one.
        inline void ConnectTo(unsigned int eSrc, avr_irq_t *irqDest) {avr_connect_irq(m_pIrq + eSrc, irqDest);}

        // Connects external IRQ to internal one.
        inline void ConnectFrom(avr_irq_t *irqSrc, unsigned int eDest) {avr_connect_irq(irqSrc, m_pIrq + eDest);}

        // Returns actual IRQ for a given enum value.
        inline avr_irq_t * GetIRQ(unsigned int eDest) {return m_pIrq + eDest;}

        // Raises your own IRQ
        void inline RasieIRQ(unsigned int eDest, uint32_t value) { avr_raise_irq(m_pIrq + eDest, value);}

        // Template to easily register and deal with C-ifying a member function.
        //typedef void (BasePeripheral::*BasePeripheralFcn)(avr_irq_t * irq, uint32_t value);

        // template<class C>
        // void RegisterNotify(unsigned int eDest, void (C::*CFcn)(avr_irq_t *irq, uint32_t value)){
        //     avr_irq_notify_t fCB = [&CFcn](avr_irq_t *irq, unsigned int value, void *param) {
        //         C *p = (C*) param;
        //         (p->*CFcn)(irq,value); 
        //         };
        //     avr_irq_register_notify(m_pIrq + eDest,fCB, this);
        //     };

        template<class C>
             void RegisterNotify(unsigned int eDest, void (C::*CFcn)(avr_irq_t *irq, uint32_t value)){
             auto fCB = [&CFcn](avr_irq_t *irq, unsigned int value, void *param) {
                 C *p = (C*) param;
                 (p->*CFcn)(irq,value); 
                 };
             avr_irq_register_notify(m_pIrq + eDest,fCB, this);
             };
             
    protected: 
        avr_irq_t *m_pIrq = nullptr;

};