#include <sim_irq.h>

// Use lambdas to expose something that can be called from C, but returns to our C++ object
// TODO: find a way to ditch the macro. I tried and failed, see the template blocks below...
#define MAKE_C_CALLBACK(class, function) \
   [](struct avr_irq_t *irq, uint32_t value, void* param) {class *p = (class*) param; p->function(irq,value); }

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

/*         template<typename T, typename R>
        void* void_cast(R(T::*f)())
        {
            union
            {
                R(T::*pf)();
                void* p[2];
            };
            pf = f;
            return p;
        }

    
        template<class C>
             void RegisterNotify(unsigned int eDest, void (C::*CFcn)(avr_irq_t *irq, uint32_t value), C* pSrc){
                typedef struct sH_t{
                    void (C::*pFcn)(avr_irq_t *irq, uint32_t value);
                    C *pClass;
                }sH_t;
                sH_t sO {pFcn:CFcn, pClass:pSrc};
                auto fCB = [](avr_irq_t *irq, unsigned int value, void *param) {
                    sH_t *p = (sH_t*)param;
                    (p->pClass->*p->pFcn)(irq,value); 
                 };
             avr_irq_register_notify(m_pIrq + eDest,fCB, &sO);
             }; */
             
    protected: 
        avr_irq_t *m_pIrq = nullptr;

};