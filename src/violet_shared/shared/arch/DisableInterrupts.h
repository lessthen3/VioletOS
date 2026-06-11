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
#ifndef VIOLET_KERNEL_ARCH_HAL_DISABLE_INTERRUPTS_HG
#define VIOLET_KERNEL_ARCH_HAL_DISABLE_INTERRUPTS_HG

#if defined(VIOLET_ARCH_X86_64)
#   define VIOLET_DISABLE_INTERRUPTS __asm__ volatile ("cli" ::: "memory")
#   define VIOLET_ENABLE_INTERRUPTS  __asm__ volatile ("sti" ::: "memory")
#elif defined(VIOLET_ARCH_ARM64)
#   define VIOLET_DISABLE_INTERRUPTS __asm__ volatile ("msr daifset, #0xf" ::: "memory")
#   define VIOLET_ENABLE_INTERRUPTS  __asm__ volatile ("msr daifclr, #0xf" ::: "memory")
#else
#   error "unsupported architecture — add interrupt disable for this arch"
#endif

#endif /*VIOLET_KERNEL_ARCH_HAL_DISABLE_INTERRUPTS_HG*/