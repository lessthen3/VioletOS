/*******************************************************************
 *                       VioletOS v0.0.1
 *        Created by Ranyodh Singh Mandur - 💜 2026-2026
 *
 *             Licensed under the MIT License (MIT).
 *        For more details, see the LICENSE file or visit:
 *              https://opensource.org/licenses/MIT
 *
 *         VioletOS is a free open source operating system
********************************************************************/
#ifndef VIOLET_KERNEL_ARCH_PLAT_SLEEP_HG
#define VIOLET_KERNEL_ARCH_PLAT_SLEEP_HG

///CSTD
#include <stdint.h>
#include <stddef.h>

///VioletShared
#include "shared/GeneralMacros.h"

#if defined(VIOLET_ARCH_X86_64)

    VIOLET_INLINE static uint64_t 
        VioletArchx86_ReadTsc(void)
    {
        uint32_t f_Low, f_High;
        __asm__ volatile ("rdtsc" : "=a"(f_Low), "=d"(f_High));
        return ((uint64_t)f_High << 32) | f_Low;
    }

    VIOLET_INLINE static void 
        VioletArch_SleepCycles(size_t fp_Cycles)
    {
        uint64_t f_Start = VioletArchx86_ReadTsc();
        while (VioletArchx86_ReadTsc() - f_Start < fp_Cycles) { ; }
    }

#elif defined(VIOLET_ARCH_ARM64)

    VIOLET_INLINE static uint64_t 
        VioletArchArm64_ReadTimer(void)
    {
        /* 
            mrs: Move to Register from System register
            cntvct_el0: counter virtual count, exception lv 0
        */ 

        uint64_t f_Ticks;
        __asm__ volatile("mrs %0, cntvct_el0" : "=r"(f_Ticks));

        return f_Ticks;
    }

    VIOLET_INLINE static void 
        VioletArch_SleepCycles(size_t fp_Cycles)
    {
        size_t f_Start = VioletArchArm64_ReadTimer();
        while (VioletArchArm64_ReadTimer() - f_Start < fp_Cycles) { ; }
    }

    VIOLET_INLINE static uint64_t 
        VioletArchArm64_GetTimerFrequency(void)
    {
        /* 
            mrs: Move to Register from System register
            cntfrq_el0: counter time frequency, exception lv 0
        */ 

        uint64_t f_Frequency;
        __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(f_Frequency));

        return f_Frequency;
    }

    VIOLET_INLINE static void 
        VioletArch_SleepMilliseconds(uint32_t fp_Milliseconds)
    {
        // calculate how many ticks we need to wait
        uint64_t f_Frequency = VioletArchArm64_GetTimerFrequency();
        uint64_t f_Ticks = (f_Frequency / 1000) * fp_Milliseconds;

        // set the countdown timer (tval)
        __asm__ volatile("msr cntv_tval_el0, %0" : : "r"(f_Ticks));

        // enable the timer (Bit 0 = Enable, Bit 1 = Unmask interrupt)
        uint32_t f_Control = 1; 
        __asm__ volatile("msr cntv_ctl_el0, %0" : : "r"(f_Control));

        // put the CPU core to sleep until the timer interrupt fires!
        __asm__ volatile("wfi");

        // we woke up! Disable the timer so it doesn't fire again
        __asm__ volatile("msr cntv_ctl_el0, xzr"); // xzr is the ARM zero register
    }

    #define VIOLET_SLEEP_FOR_CYCLES(fp_SleepCycles) VioletArchArm64_SleepCycles(fp_SleepCycles)

#else
#   error "VioletOS: Architecture not supported for sleep loops!"
#endif /* Arch Selection */


#endif /*VIOLET_KERNEL_ARCH_PLAT_SLEEP_HG*/