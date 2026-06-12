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
#ifndef VIOLET_KERNEL_VIRTUAL_MEMORY_MANAGER_HG
#define VIOLET_KERNEL_VIRTUAL_MEMORY_MANAGER_HG

///VioletShared
#include "shared/BootInfo.h"
#include "shared/arch/MemoryWidthTypes.h"
#include "shared/GeneralMacros.h"

///VioletKernel

///CSTD
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*============================================================
    Virtual Address Space Layout

    0x0000000000001000 — 0x00007FFFFFFFFFFF   user space (128TB)
    0xFFFF800000000000 — 0xFFFFBFFFFFFFFFFF   direct physical map (64TB) -- every byte of physical RAM is accessible at DIRECT_MAP_BASE + phys_addr
    0xFFFFFF8000000000 — 0xFFFFFFFFFFFFFFFF   kernel virtual space
    0xFFFFFFFF80000000                        kernel image base (VMA in the linker script)
==============================================================*/

#define VIOLET_USER_SPACE_START       0x0000000000001000ULL
#define VIOLET_USER_SPACE_END         0x00007FFFFFFFF000ULL

#define VIOLET_DIRECT_MAP_BASE        0xFFFF800000000000ULL
#define VIOLET_DIRECT_MAP_END         0xFFFFBFFFFFFFFFFFULL

#define VIOLET_KERNEL_SPACE_START     0xFFFFFF8000000000ULL
#define VIOLET_KERNEL_IMAGE_BASE      0xFFFFFFFF80000000ULL

/*============================================================
    Page table entry flags
    identical to the bootloader flags — redeclared here so
    kernel code doesn't need to include bootloader headers
==============================================================*/

#define VMM_FLAG_PRESENT              (1ULL << 0)
#define VMM_FLAG_WRITABLE             (1ULL << 1)
#define VMM_FLAG_USER                 (1ULL << 2)   /* accessible from ring 3 */
#define VMM_FLAG_WRITE_THROUGH        (1ULL << 3)
#define VMM_FLAG_CACHE_DISABLE        (1ULL << 4)   /* use for MMIO mappings */
#define VMM_FLAG_ACCESSED             (1ULL << 5)   /* set by CPU on access */
#define VMM_FLAG_DIRTY                (1ULL << 6)   /* set by CPU on write */
#define VMM_FLAG_HUGE                 (1ULL << 7)   /* 2MB page at PD level */
#define VMM_FLAG_GLOBAL               (1ULL << 8)   /* survives CR3 reload, use for kernel pages */
#define VMM_FLAG_NO_EXECUTE           (1ULL << 63)

/* common flag combinations */
#define VMM_FLAGS_KERNEL_CODE         (VMM_FLAG_PRESENT | VMM_FLAG_GLOBAL)
#define VMM_FLAGS_KERNEL_DATA         (VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE | VMM_FLAG_GLOBAL | VMM_FLAG_NO_EXECUTE)
#define VMM_FLAGS_KERNEL_RODATA       (VMM_FLAG_PRESENT | VMM_FLAG_GLOBAL   | VMM_FLAG_NO_EXECUTE)
#define VMM_FLAGS_MMIO                (VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE | VMM_FLAG_CACHE_DISABLE | VMM_FLAG_NO_EXECUTE)
#define VMM_FLAGS_USER_CODE           (VMM_FLAG_PRESENT | VMM_FLAG_USER)
#define VMM_FLAGS_USER_DATA           (VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE | VMM_FLAG_USER | VMM_FLAG_NO_EXECUTE)

/*============================================================
    Physical address mask — strips flag bits from a page table entry
    to get the physical address of the next-level table
==============================================================*/

#define VMM_PHYSICAL_ADDRESS_MASK     0x000FFFFFFFFFF000ULL

/*============================================================
    VioletVmm_VmaRegion — one contiguous virtual memory allocation
    tracks what's mapped and how so we can unmap it later

    max regions per address space is intentionally bounded —
    upgrade to a dynamic structure once we have kmalloc
==============================================================*/

#define VMM_MAX_REGIONS_PER_SPACE     256

typedef struct {
    virt_addr_t Base;           /* start of this virtual region */
    size_t      PageCount;      /* how many pages */
    uint64_t    Flags;          /* the flags this region was mapped with */
    bool        IsAllocated;    /* true = this slot is in use */
} VioletVmm_VmaRegion;

/*============================================================
    VioletVmm_AddressSpace — a complete virtual address space
    one per process, plus the kernel address space

    Pml4PhysAddr: physical address of the PML4 table load this into CR3 to switch to this space
    Regions:      VMA region tracking, static pool for now
    RegionCount:  how many slots are in use
    IsKernel:     true = this is the kernel address space, kernel mappings are shared across all spaces
==============================================================*/

typedef struct {
    phys_addr_t          Pml4PhysAddr;
    VioletVmm_VmaRegion  Regions[VMM_MAX_REGIONS_PER_SPACE];
    size_t               RegionCount;
    bool                 IsKernel;
} VioletVmm_AddressSpace;

/*============================================================
    VMM interface
==============================================================*/

/*
    VioletVmm_Init — initialise the VMM from boot info
    takes ownership of the bootloader's PML4
    builds the direct physical map so PMM pages are accessible
    sets up the kernel address space structure
    must be called after VioletPmm_Init
*/
void 
    VioletVmm_Init
    (
        const VioletBoot_Info* fp_BootInfo
    );

/*
    VioletVmm_GetKernelSpace — returns a pointer to the kernel address space
    the kernel address space is the one currently loaded in CR3 on boot
*/

VioletVmm_AddressSpace* 
    VioletVmm_GetKernelSpace(void);

/*
    VioletVmm_PhysToVirt — convert a physical address to its direct-map virtual address
    only valid for addresses below DIRECT_MAP_END - DIRECT_MAP_BASE (64TB)
    use for accessing PMM-allocated pages as kernel virtual addresses
*/

static VIOLET_INLINE virt_addr_t 
    VioletVmm_PhysicalToVirtualAddress
    (
        phys_addr_t fp_PhysAddr
    )
{
    return (virt_addr_t)(VIOLET_DIRECT_MAP_BASE + fp_PhysAddr);
}

/*
    VioletVmm_VirtToPhys — walk page tables to find the physical address
    for a virtual address in the given address space
    returns 0 if the address is not mapped
*/

phys_addr_t 
    VioletVmm_VirtualToPhysicalAddress
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtAddr
    );

/*
    VioletVmm_MapPage — map one virtual page to one physical page
    fp_Space:    address space to modify
    fp_VirtAddr: virtual address (must be 4KB aligned)
    fp_PhysAddr: physical address (must be 4KB aligned)
    fp_Flags:    VMM_FLAG_* combination

    allocates intermediate page table pages from the PMM as needed
    returns true on success
*/
bool 
    VioletVmm_MapPage
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtAddr,
        phys_addr_t             fp_PhysAddr,
        uint64_t                fp_Flags
    );

/*
    VioletVmm_UnmapPage — remove a virtual page mapping
    fp_FreePhysical: if true, also frees the physical page via PMM
    does nothing if the page is not mapped
*/
void 
    VioletVmm_UnmapPage
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtAddr,
        bool                    fp_FreePhysical
    );

/*
    VioletVmm_MapRange — map a contiguous virtual range to a contiguous physical range
    convenience wrapper around VioletVmm_MapPage
    fp_PageCount: number of 4KB pages to map
*/

bool 
    VioletVmm_MapRange
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtBase,
        phys_addr_t             fp_PhysBase,
        size_t                  fp_PageCount,
        uint64_t                fp_Flags
    );

/*
    VioletVmm_UnmapRange — unmap a contiguous virtual range
    fp_FreePhysical: if true, frees each physical page via PMM
*/

void 
    VioletVmm_UnmapRange
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtBase,
        size_t                  fp_PageCount,
        bool                    fp_FreePhysical
    );

/*
    VioletVmm_AllocatePages — allocate fp_PageCount pages of virtual + physical memory
    finds a free virtual range in fp_Space
    allocates physical pages from PMM
    maps them with fp_Flags
    records the allocation in the space's region list
    returns the virtual base address, 0 on failure
*/
virt_addr_t 
    VioletVmm_AllocatePages
    (
        VioletVmm_AddressSpace* fp_Space,
        size_t                  fp_PageCount,
        uint64_t                fp_Flags
    );

/*
    VioletVmm_FreePages — free a virtual allocation made by VioletVmm_AllocatePages
    unmaps the virtual range and returns physical pages to PMM
    fp_VirtBase must be the address returned by VioletVmm_AllocatePages
*/
void 
    VioletVmm_FreePages
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtBase
    );

/*
    VioletVmm_CreateAddressSpace — create a new empty address space
    allocates a new PML4 page from PMM
    copies kernel PML4 entries into the upper half so kernel mappings
    are shared across all address spaces (standard higher-half kernel design)
    returns pointer to the new space, nullptr on failure
*/
VioletVmm_AddressSpace* 
    VioletVmm_CreateAddressSpace(void);

/*
    VioletVmm_DestroyAddressSpace — tear down an address space
    unmaps and frees all user-space pages and page table pages
    does NOT free kernel mappings (those are shared)
    do not call this on the kernel address space
*/
void 
    VioletVmm_DestroyAddressSpace(VioletVmm_AddressSpace* fp_Space);

/*
    VioletVmm_SwitchAddressSpace — load fp_Space's PML4 into CR3
    flushes the TLB for all non-global pages
    global pages (VMM_FLAG_GLOBAL) survive the switch — use for kernel mappings
*/
void 
    VioletVmm_SwitchAddressSpace(const VioletVmm_AddressSpace* fp_Space);

#endif /*VIOLET_KERNEL_VIRTUAL_MEMORY_MANAGER_HG*/