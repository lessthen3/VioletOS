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
#ifndef VIOLET_KERNEL_INTERRUPT_DESCRIPTOR_TABLE_HG
#define VIOLET_KERNEL_INTERRUPT_DESCRIPTOR_TABLE_HG

#include "shared/GeneralMacros.h"

#if defined(VIOLET_ARCH_X86_64)

static VIOLET_INLINE void
    VioletArch_SetupInterruptDescriptorTable()
{

}

#elif defined(VIOLET_ARCH_ARM64)

static VIOLET_INLINE void
    VioletArch_SetupInterruptDescriptorTable()
{
    
}

#else
#   error "unsupported architecture add IDT support for this arch ;w;"
#endif

#endif /*VIOLET_KERNEL_INTERRUPT_DESCRIPTOR_TABLE_HG*/