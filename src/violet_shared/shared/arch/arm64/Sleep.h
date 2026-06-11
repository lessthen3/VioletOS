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
#ifndef VIOLET_KERNEL_ARCH_ARM64_SLEEP_HG
#define VIOLET_KERNEL_ARCH_ARM64_SLEEP_HG

///CSTD
#include <stdint.h>

///VioletShared
#include "shared/GeneralMacros.h"

VIOLET_FORCEINLINE static uint64_t 
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

VIOLET_FORCEINLINE static void 
    VioletArchArm64_SleepCycles(uint64_t fp_Cycles)
{
    uint64_t f_Start = VioletArchArm64_ReadTimer();
    while (VioletArchArm64_ReadTimer() - f_Start < fp_Cycles) { ; }
}

VIOLET_FORCEINLINE static uint64_t 
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

VIOLET_FORCEINLINE static void 
    VioletArchArm64_SleepMilliseconds(uint32_t fp_Milliseconds)
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

#endif /*VIOLET_KERNEL_ARCH_ARM64_SLEEP_HG*/