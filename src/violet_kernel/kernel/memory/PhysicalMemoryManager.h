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
#ifndef VIOLET_KERNEL_PHYSICAL_MEMORY_MANAGER_HG
#define VIOLET_KERNEL_PHYSICAL_MEMORY_MANAGER_HG

#include <stdint.h>
#include <stddef.h>

///VioletShared
#include "shared/BootInfo.h"

/*
    VioletPMM — the physical memory allocator state
    one global instance lives in kernel BSS

    Bitmap:          one bit per physical page, 1=used 0=free
                    allocated from the first usable conventional memory region
                    during PMM_Init before the allocator is live
    TotalPages:      total pages in the physical address space we manage
    FreePages:       current count of free pages, maintained on alloc/free
    LastSearchIndex: where the last successful allocation search ended
                    next search starts here instead of bit 0 every time
                    makes repeated allocations O(1) amortised instead of O(n)
*/
typedef struct {
    size_t* Bitmap;
    size_t  TotalPages;
    size_t  FreePages;
    size_t  LastSearchIndex;
} Violet_Pmm;

/*
    VioletPMM_Init — initialise the PMM from the UEFI memory map
    carves the bitmap out of the first usable conventional memory region
    marks all regions according to UEFI memory types
    then marks kernel + bitmap as used on top of that

    fp_BootInfo:      full boot info from bootloader
    fp_KernelPhysEnd: first byte past the kernel image (VioletKernelEnd physical)
                    everything from KernelPhysBase to here is marked used
*/
void 
    VioletPmm_Init
    (
        const VioletBoot_Info* fp_BootInfo,
        size_t                 fp_KernelPhysEnd
    );

/*
    VioletPMM_AllocatePage — allocate one physical 4KB page
    returns physical address of the page, 0 on failure (out of memory)
    the returned page is zeroed
*/
uint64_t 
    VioletPmm_AllocatePage();

/*
    VioletPMM_AllocatePages — allocate fp_Count contiguous physical pages
    returns physical address of the first page, 0 on failure
    contiguous allocation needed for page tables, DMA buffers etc
    the returned pages are NOT zeroed — caller must zero if needed
*/
uint64_t 
    VioletPmm_AllocatePages(size_t fp_Count);

/*
    VioletPMM_FreePage — free one physical page
    fp_PhysAddr must be 4KB aligned and previously allocated
*/
void 
    VioletPmm_FreePage(size_t fp_PhysicalAddress);

/*
    VioletPMM_FreePages — free fp_Count contiguous pages starting at fp_PhysAddr
*/
void 
    VioletPmm_FreePages
    (
        size_t fp_PhysicalAddress, 
        size_t fp_Count
    );

/*
    VioletPMM_GetFreePageCount — how many pages are currently free
*/
uint64_t 
    VioletPmm_GetFreePageCount();

/*
    VioletPMM_GetTotalPageCount — total pages under PMM management
*/
uint64_t 
    VioletPmm_GetTotalPageCount();

#endif /*VIOLET_KERNEL_PHYSICAL_MEMORY_MANAGER_HG*/