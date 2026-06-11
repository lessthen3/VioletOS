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
#ifndef VIOLET_KERNEL_ARCH_X86_SLEEP_HG
#define VIOLET_KERNEL_ARCH_X86_SLEEP_HG

///CSTD
#include <stdint.h>

///VioletShared
#include "shared/GeneralMacros.h"

VIOLET_FORCEINLINE static uint64_t 
    VioletArchx86_ReadTsc(void)
{
    uint32_t f_Low, f_High;
    __asm__ volatile ("rdtsc" : "=a"(f_Low), "=d"(f_High));
    return ((uint64_t)f_High << 32) | f_Low;
}

VIOLET_FORCEINLINE static void 
    VioletArchx86_SleepCycles(uint64_t fp_Cycles)
{
    uint64_t f_Start = VioletArchx86_ReadTsc();
    while (VioletArchx86_ReadTsc() - f_Start < fp_Cycles) { ; }
}

#endif /*VIOLET_KERNEL_ARCH_X86_SLEEP_HG*/