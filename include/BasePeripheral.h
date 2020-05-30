/*
	BasePeripheral.h - a base class for any peripheral used with Mk3Sim

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK3SIM.

	MK3SIM is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK3SIM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK3SIM.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <sim_avr.h>
#include <sim_irq.h>

#ifndef __BASE_PERIPHERAL_H__
#define __BASE_PERIPHERAL_H__

// Use lambdas to expose something that can be called from C, but returns to our C++ object
// TODO: find a way to ditch the macro. I tried and failed, see the template blocks below...

// Generates a lambda function inline that can be called from SimAVR's C IRQ code.
#define MAKE_C_CALLBACK(class, function) \
   [](struct avr_irq_t *irq, uint32_t value, void* param) {class *p = (class*) param; p->function(irq,value); }

// Generates an inline lambda for use with aver_cycle_timer
#define MAKE_C_TIMER_CALLBACK(class, function) \
   [](avr_t * avr, avr_cycle_count_t when, void* param) {class *p = (class*) param; return p->function(avr,when); }


class BasePeripheral 
{
    public:

        // Returns actual IRQ for a given enum value.
        inline avr_irq_t * GetIRQ(unsigned int eDest) {return m_pIrq + eDest;}

        // Connects internal IRQ to an external one.
        inline void ConnectTo(unsigned int eSrc, avr_irq_t *irqDest) {avr_connect_irq(m_pIrq + eSrc, irqDest);}

        // Connects external IRQ to internal one.
        inline void ConnectFrom(avr_irq_t *irqSrc, unsigned int eDest) {avr_connect_irq(irqSrc, m_pIrq + eDest);}

    protected:

        // Sets up the IRQs on "avr" for this class. Optional name override IRQNAMES.
        template<class C>
        void _Init(avr_t *avr, C *p, const char** IRQNAMES = nullptr) {
            m_pAVR = avr;
            if (IRQNAMES)
                m_pIrq = avr_alloc_irq(&avr->irq_pool,0,p->COUNT,IRQNAMES);
            else
                m_pIrq = avr_alloc_irq(&avr->irq_pool,0,p->COUNT,p->_IRQNAMES);
         };

        // Raises your own IRQ
        void inline RaiseIRQ(unsigned int eDest, uint32_t value) { avr_raise_irq(m_pIrq + eDest, value);}
        void inline RaiseIRQFloat(unsigned int eDest, uint32_t value) { avr_raise_irq_float(m_pIrq + eDest, value,m_pIrq->flags | IRQ_FLAG_FLOATING);}

        // Registers an IRQ notification function. Use MAKE_C_CALLBACK to make a lambda function.
        template <class C>
        void inline RegisterNotify(unsigned int eSrc, avr_irq_notify_t func, C* pObj) { avr_irq_register_notify(m_pIrq + eSrc, func, pObj); };

        // Cancels a registered cycle timer.
        template <class C>
        void inline CancelTimer(avr_cycle_timer_t func, C* pObj) { avr_cycle_timer_cancel(m_pAVR, func, pObj); };

        // Registers a callback for a cycle timer, in usec
        template <class C>
        void inline RegisterTimerUsec(avr_cycle_timer_t func, uint32_t uiUsec, C* pObj) { avr_cycle_timer_register_usec(m_pAVR, uiUsec, func, pObj); };

        // Registers a callback for a cycle timer, in cycles.
        template <class C>
        void inline RegisterTimer(avr_cycle_timer_t func, uint32_t uiCycles, C* pObj) { avr_cycle_timer_register(m_pAVR, uiCycles, func, pObj); };

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
             
        struct avr_t *m_pAVR = nullptr;
        avr_irq_t * m_pIrq = nullptr;

};

#endif /* __BASE_PERIPHERAL_H__ */