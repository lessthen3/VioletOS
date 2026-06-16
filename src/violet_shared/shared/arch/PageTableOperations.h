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
#ifndef VIOLET_SHARED_ARCH_PAGE_TABLE_OPERATIONS_HG
#define VIOLET_SHARED_ARCH_PAGE_TABLE_OPERATIONS_HG

#include "shared/arch/Memory.h"
#include "shared/GeneralMacros.h"

//cant tell if i like inline static or static inline better, i think static inline uwu

#if defined(VIOLET_ARCH_X86_64)

    VIOLET_INLINE static void 
        VioletArch_LoadPageTable(phys_addr_t fp_PhysAddr)
    {
        __asm__ volatile 
        (
            "mov cr3, %0" 
            : : "r"(fp_PhysAddr) 
            : "memory"
        );
    }

    VIOLET_INLINE static phys_addr_t 
        VioletArch_ReadPageTable()
    {
        phys_addr_t f_Value;
        
        __asm__ volatile 
        (
            "mov %0, cr3" 
            : "=r"(f_Value)
        );

        return f_Value;
    }

    static VIOLET_INLINE void 
        VioletArch_InvalidatePage(virt_addr_t fp_VirtualAddress)
    {
        __asm__ volatile ("invlpg [%0]" : : "r"(fp_VirtualAddress) : "memory");
    }

    static VIOLET_INLINE void 
        VioletArch_FlushTlb()
    {
        __asm__ volatile
        (
            "mov rax, cr3\n\t"
            "mov cr3, rax"
            : : : "rax", "memory"
        );
    }

#elif defined(VIOLET_ARCH_ARM64)

    VIOLET_INLINE static void 
        VioletArch_LoadPageTable(phys_addr_t fp_PhysAddr)
    {
        __asm__ volatile ("msr ttbr1_el1, %0\n\t isb" : : "r"(fp_PhysAddr) : "memory");
    }

    VIOLET_INLINE static phys_addr_t 
        VioletArch_ReadPageTable()
    {
        phys_addr_t f_Value;
        __asm__ volatile ("mrs %0, ttbr1_el1" : "=r"(f_Value));

        return f_Value;
    }

    VIOLET_INLINE static void 
        VioletArch_InvalidatePage(virt_addr_t fp_VirtAddr)
    {
        /*
            tlbi vae1is: TLB Invalidate by VA, EL1, Inner Shareable
            shifts right 12 to strip page offset before passing to the instruction
        */
        __asm__ volatile ("tlbi vae1is, %0\n\t dsb sy\n\t isb" : : "r"(fp_VirtAddr >> 12) : "memory");
    }

    VIOLET_INLINE static void 
        VioletArch_FlushTlb()
    {
        __asm__ volatile 
        (
            "dsb ishst\n\t"
            "tlbi vmalle1is\n\t"
            "dsb ish\n\t"
            "isb"
            : : : "memory"
        );
    }

#else
#   error "VioletOS: unsupported architecture you need to add PageTableOps for this arch"
#endif


#endif /*VIOLET_KERNEL_ARCH_X86_SLEEP_HG*/