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

#if defined(VIOLET_ARCH_X86_64)
#   include "shared/arch/x86/Sleep.h"
#   define VIOLET_SLEEP_FOR_CYCLES(fp_SleepCycles) VioletArchx86_SleepCycles(fp_SleepCycles)
#elif defined(VIOLET_ARCH_ARM64)
#   include "shared/arch/arm64/Sleep.h"
#   define VIOLET_SLEEP_FOR_CYCLES(fp_SleepCycles) VioletArchArm64_SleepCycles(fp_SleepCycles)
#else
#   error "VioletOS: Architecture not supported for sleep loops!"
#endif /* Arch Selection */


#endif /*VIOLET_KERNEL_ARCH_PLAT_SLEEP_HG*/