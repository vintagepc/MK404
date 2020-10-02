/*
	BasePeripheral.h - a base class for any peripheral used with MK404

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

#include "gsl-lite.hpp"
#include "sim_avr.h"
#include "sim_irq.h"
#include <array>
#include <iostream>
#include <utility>


// Use lambdas to expose something that can be called from C, but returns to our C++ object
// TODO(anyone): find a way to ditch the macro. I tried and failed, see the template blocks below...

// Generates a lambda function inline that can be called from SimAVR's C IRQ code.
#define MAKE_C_CALLBACK(class, function) \
   [](struct avr_irq_t *irq, uint32_t value, void* param) {auto *p = static_cast<class*>(param); p->function(irq,value); }

// Generates an inline lambda for use with aver_cycle_timer
#define MAKE_C_TIMER_CALLBACK(class, function) \
   [](avr_t * avr, avr_cycle_count_t when, void* param) {auto *p = static_cast<class*>(param); return p->function(avr,when); }


class BasePeripheral
{
    public:
        enum IRQ : unsigned int;

        // Returns actual IRQ for a given enum value.
        inline avr_irq_t * GetIRQ(unsigned int eDest) {return m_pIrq.begin() + eDest;}

        // Connects internal IRQ to an external one.
        inline void ConnectTo(unsigned int eSrc, avr_irq_t *irqDest)
		{
			avr_connect_irq(m_pIrq.begin() + eSrc, irqDest);
			StashIRQs(m_pIrq.begin() + eSrc, irqDest);
		}

        // Connects external IRQ to internal one.
        inline void ConnectFrom(avr_irq_t *irqSrc, unsigned int eDest)
		{
			avr_connect_irq(irqSrc, m_pIrq.begin() + eDest);
			StashIRQs(irqSrc, m_pIrq.begin() + eDest);

		}

		// Detaches the component from its registered IRQs. They must have come through ConnectTo/From!
		void DisconnectAll()
		{
			if (!m_bCanDetach)
			{
				std::cout << "Cannot disconnect component, it has registered too many IRQs (>15).\n";
				return;
			}
			for (auto it = m_vIrqs.begin(); it!=m_vIrqs.end(); it+=2)
			{
				avr_unconnect_irq(*it, *(it+1));
			}
		}

		// Disconnects the hardware's IRQs.
		// void Disconnect()
		// {
		// 	for (auto &c: m_vIrqs)
		// 	{
		// 		avr_unconnect_irq(c.first, c.second);
		// 	}
		// 	m_vIrqs.clear();
		// }

    protected:

        // Sets up the IRQs on "avr" for this class. Optional name override IRQNAMES.
        template<class C, typename... Args>
        void _InitWithArgs(avr_t *avr, C *p,  const char** IRQNAMES, Args... args) {
            m_pAVR = avr;
            if (IRQNAMES)
			{
				_m_pIrq = avr_alloc_irq(&avr->irq_pool,0,p->COUNT,IRQNAMES);
			}
            else
			{
                _m_pIrq = avr_alloc_irq(&avr->irq_pool,0,p->COUNT,static_cast<const char**>(p->_IRQNAMES));
			}
			m_pIrq = {_m_pIrq,p->COUNT};
			p->OnPostInit(avr, args...);
         };

		template<class C>
			void _Init(avr_t *avr, C *p, const char** IRQNAMES = nullptr) {
				m_pAVR = avr;
				if (IRQNAMES)
				{
					_m_pIrq = avr_alloc_irq(&avr->irq_pool,0,p->COUNT,IRQNAMES);
				}
				else
				{
					_m_pIrq = avr_alloc_irq(&avr->irq_pool,0,p->COUNT,static_cast<const char**>(p->_IRQNAMES));
				}
				m_pIrq = {_m_pIrq,p->COUNT};
			};

        // Raises your own IRQ
        void inline RaiseIRQ(unsigned int eDest, uint32_t value) { avr_raise_irq(m_pIrq.begin() + eDest, value);}
        void inline RaiseIRQFloat(unsigned int eDest, uint32_t value) { avr_raise_irq_float(m_pIrq.begin() + eDest, value, (m_pIrq.begin() + eDest)->flags & IRQ_FLAG_FLOATING);}

        // Registers an IRQ notification function. Use MAKE_C_CALLBACK to make a lambda function.
        template <class C>
        void inline RegisterNotify(unsigned int eSrc, avr_irq_notify_t func, C* pObj) { avr_irq_register_notify(m_pIrq.begin() + eSrc, func, pObj); };

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
        avr_irq_t * _m_pIrq = nullptr;
		gsl::span<avr_irq_t> m_pIrq {};
        struct avr_t *m_pAVR = nullptr;
    private:

		inline void StashIRQs(avr_irq_t *p1, avr_irq_t *p2)
		{
			if (m_irqCt>28)
			{
				std::cout << ">15 IRQs on peripheral, it can no longer be detached cleanly.\n";
				m_bCanDetach = false;
				return;
			}
			gsl::at(m_vIrqs,m_irqCt++) = p1;
			gsl::at(m_vIrqs,m_irqCt++) = p2;
		}

		// Can't use vector because some derivatives have atomic members...
		std::array<avr_irq_t*,30> m_vIrqs {};
		int m_irqCt = 0;
		bool m_bCanDetach = true;

};
