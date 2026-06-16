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
#ifndef VIOLET_BOOTLOADER_PAGE_TABLE_HG
#define VIOLET_BOOTLOADER_PAGE_TABLE_HG

///UEFI
#include <Uefi.h>

///VioletBootloader
#include "ElfLoader.h"

///CSTD
#include <stdint.h>

/*
    these bits are set in each entry regardless of which level it's at
*/
#define PAGE_TABLE_ENTRY_PRESENT     (1ULL << 0)   /* entry is valid */
#define PAGE_TABLE_ENTRY_WRITABLE    (1ULL << 1)   
#define PAGE_TABLE_ENTRY_HUGE        (1ULL << 7)   /* PD entry: 2MB page, no PT level */
#define PAGE_TABLE_ENTRY_NO_EXECUTE  (1ULL << 63)  

/*
    VioletSetupPageTables — allocate and populate minimal page tables
    maps kernel VMA (0xFFFFFFFF80000000) → physical load address
    maps identity (physical load address → same) for safe CR3 load

    fp_SystemTable:      for AllocatePages
    fp_KernelPhysBase:   where ELF loader put the kernel (0x200000)
    fp_KernelVirtBase:   kernel VMA from linker script (0xFFFFFFFF80000000)
    fp_KernelPageCount:  how many 4KB pages the kernel occupies

    returns physical address of PML4 — load this into CR3
    returns 0 on failure
*/
uint64_t 
    Violet_InitializeRootPml4
    (
        EFI_SYSTEM_TABLE* fp_SystemTable
    );


EFI_STATUS 
    Violet_MapPage
    (
        EFI_SYSTEM_TABLE* fp_SystemTable,
        uint64_t          fp_Pml4,
        uint64_t          fp_VirtualAddress,
        uint64_t          fp_PhysicalAddress,
        uint64_t          fp_Flags
    );

EFI_STATUS
    VioletPageTable_SetupKernelPageTable
    (
        EFI_SYSTEM_TABLE* fp_SystemTable,
        uint64_t          fp_Pml4,
        VioletLoadedKernel fp_LoadedKernel,
        VioletGop_Console  fp_Console
    );

#endif /*VIOLET_BOOTLOADER_PAGE_TABLE_HG*/