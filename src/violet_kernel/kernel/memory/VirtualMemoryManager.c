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
#include "VirtualMemoryManager.h"

///VioletKernel
#include "kernel/VioletPanic.h"
#include "kernel/vklib/memset.h"
#include "kernel/memory/PhysicalMemoryManager.h"

///VioletShared
#include "shared/arch/PageTableOperations.h"

///CSTD
#include <iso646.h>

/*============================================================
    UEFI memory type constants
    mirrored here so we don't need UEFI headers in the kernel           IM JUST GONNA COPY PASTE IT HERE CAUSE WHATEVER UWU they're macros anyways
    values are from the UEFI spec section 7.2, table 7.6 (2.11)
==============================================================*/

#define EFI_RESERVED_MEMORY_TYPE        0
#define EFI_LOADER_CODE                 1
#define EFI_LOADER_DATA                 2
#define EFI_BOOT_SERVICES_CODE          3
#define EFI_BOOT_SERVICES_DATA          4
#define EFI_RUNTIME_SERVICES_CODE       5
#define EFI_RUNTIME_SERVICES_DATA       6
#define EFI_CONVENTIONAL_MEMORY         7
#define EFI_UNUSABLE_MEMORY             8
#define EFI_ACPI_RECLAIM_MEMORY         9
#define EFI_ACPI_MEMORY_NVS             10
#define EFI_MEMORY_MAPPED_IO            11
#define EFI_MEMORY_MAPPED_IO_PORT_SPACE 12
#define EFI_PAL_CODE                    13
#define EFI_PERSISTENT_MEMORY           14
#define EFI_UNACCEPTED_MEMORY_TYPE      15

/*============================================================
    kernel address space — one global instance in BSS
    the bootloader's PML4 becomes this after VioletVmm_Init
==============================================================*/

static VioletVmm_AddressSpace s_KernelAddressSpace;

/*============================================================
    address space pool — static pool for early process support
    before we have kmalloc for dynamic allocation
    slot 0 is always the kernel space
==============================================================*/

#define VMM_MAX_ADDRESS_SPACES 64

static VioletVmm_AddressSpace s_AddressSpacePool[VMM_MAX_ADDRESS_SPACES];
static bool                   s_AddressSpacePoolUsed[VMM_MAX_ADDRESS_SPACES];

/*============================================================
    x86_64 page table index extraction
    virtual address bits:
        [63:48] sign extension
        [47:39] PML4 index
        [38:30] PDPT index
        [29:21] PD index
        [20:12] PT index
        [11:0]  page offset
==============================================================*/

static inline uint64_t 
    Pml4Index(virt_addr_t fp_VirtAddr)
{
    return (fp_VirtAddr >> 39) & 0x1FF;
}

static inline uint64_t 
    PdptIndex(virt_addr_t fp_VirtAddr)
{
    return (fp_VirtAddr >> 30) & 0x1FF;
}

static inline uint64_t 
    PdIndex(virt_addr_t fp_VirtAddr)
{
    return (fp_VirtAddr >> 21) & 0x1FF;
}

static inline uint64_t 
    PtIndex(virt_addr_t fp_VirtAddr)
{
    return (fp_VirtAddr >> 12) & 0x1FF;
}

/*============================================================
    get a pointer to a page table given its physical address
    uses the direct physical map — valid only after VioletVmm_Init
    sets up the direct map region
==============================================================*/

static inline uint64_t* 
    Internal_PhysicalToVirtualAdress(phys_addr_t fp_PhysicalAddress)
{
    return (uint64_t*)(VIOLET_DIRECT_MAP_BASE + fp_PhysicalAddress);
}

/*============================================================
    allocate one zeroed page table page from the PMM
    returns physical address, 0 on failure
==============================================================*/

static phys_addr_t 
    AllocateTablePage()
{
    phys_addr_t f_PhysAddr = VioletPmm_AllocatePage();

    VIOLET_PANIC_IF(f_PhysAddr == 0, "VMM ran out of physical memory for page table"); //TODO: REPLACE ERROR SHOULDNT PANIC HERE JUST KILL PROCESS OWO OR SWAP

    /*
        VioletPmm_AllocatePage returns a zeroed page the PMM zeros it using the direct map
    */
    uint64_t* f_Table = Internal_PhysicalToVirtualAdress(f_PhysAddr);

    for (int lv_Index = 0; lv_Index < 512; lv_Index++)
    {
        f_Table[lv_Index] = 0;
    }

    return f_PhysAddr;
}

/*============================================================
    get or create the next-level page table entry
    walks one level of the page table tree
    creates missing intermediate tables as needed

    fp_TablePhys: physical address of the current level table
    fp_Index:     index into that table
    fp_Flags:     flags to set on a newly created entry
                (flags on existing entries are NOT modified)

    returns physical address of the next-level table
    returns 0 on allocation failure
==============================================================*/

static phys_addr_t 
    GetOrCreateNextTable
    (
        phys_addr_t fp_TablePhys,
        uint64_t    fp_Index,
        uint64_t    fp_Flags
    )
{
    uint64_t* f_Table = Internal_PhysicalToVirtualAdress(fp_TablePhys);
    uint64_t  f_Entry = f_Table[fp_Index];

    if (f_Entry & VMM_FLAG_PRESENT)
    {
        /*
            entry already exists — return the physical address it points to
            mask off the flag bits to get the clean address
        */
        return (phys_addr_t)(f_Entry & VMM_PHYSICAL_ADDRESS_MASK);
    }

    /*
        entry missing — allocate a new table page and create the entry
        intermediate entries (PML4/PDPT/PD) always get writable + user flags
        so that user mappings can be placed below them
        the final PT entry carries the actual permission flags
    */
    phys_addr_t f_NewTable = AllocateTablePage();

    if (f_NewTable == 0)
    {
        return 0;
    }

    f_Table[fp_Index] = f_NewTable | fp_Flags | VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE;

    return f_NewTable;
}

/*============================================================
    VioletVmm_MapPage
==============================================================*/

bool 
    VioletVmm_MapPage
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtualAddress,
        phys_addr_t             fp_PhysicalAddress,
        uint64_t                fp_Flags
    )
{
    VIOLET_PANIC_IF(fp_Space == nullptr,                         "nullptr reference to vmm address space was passed ;w;");
    VIOLET_PANIC_IF(fp_VirtualAddress &  (VIOLET_PAGE_SIZE - 1), "virtual address not page aligned >:^(");
    VIOLET_PANIC_IF(fp_PhysicalAddress & (VIOLET_PAGE_SIZE - 1), "physical address not page aligned >:^(");

    /*
        walk down the 4 levels of the page table tree
        creating intermediate tables as needed
        the intermediate entries get USER flag if the mapping is user-accessible
        so the CPU allows traversal at ring 3
    */
    uint64_t f_IntermediateFlags = (fp_Flags & VMM_FLAG_USER) ? VMM_FLAG_USER : 0;

    phys_addr_t f_Pdpt = GetOrCreateNextTable(fp_Space->Pml4PhysicalAddress, Pml4Index(fp_VirtualAddress), f_IntermediateFlags);

    if (f_Pdpt == 0) 
    { 
        return false;
    }

    phys_addr_t f_Pd = GetOrCreateNextTable(f_Pdpt, PdptIndex(fp_VirtualAddress), f_IntermediateFlags);

    if (f_Pd == 0) 
    { 
        return false; 
    }

    phys_addr_t f_Pt = GetOrCreateNextTable(f_Pd, PdIndex(fp_VirtualAddress), f_IntermediateFlags);

    if (f_Pt == 0) 
    { 
        return false;
    }

    /*
        write the final PT entry with the actual flags
        if an entry already exists here we overwrite it — this is intentional
        for remapping with different flags
    */
    uint64_t* f_PtTable = Internal_PhysicalToVirtualAdress(f_Pt);
    uint64_t  f_OldEntry = f_PtTable[PtIndex(fp_VirtualAddress)];

    f_PtTable[PtIndex(fp_VirtualAddress)] = fp_PhysicalAddress | fp_Flags;

    /*
        if we're replacing an existing mapping, flush the TLB entry
        so the CPU doesn't use the stale cached translation
    */
    if (f_OldEntry & VMM_FLAG_PRESENT)
    {
        VioletArch_InvalidatePage(fp_VirtualAddress);
    }

    return true;
}

/*============================================================
    VioletVmm_UnmapPage
==============================================================*/

void 
    VioletVmm_UnmapPage
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtualAddress,
        bool                    fp_FreePhysical
    )
{
    if (fp_Space == nullptr)
    {
        return;
    }

    uint64_t* f_Pml4 = Internal_PhysicalToVirtualAdress(fp_Space->Pml4PhysicalAddress);
    uint64_t  f_Pml4Entry = f_Pml4[Pml4Index(fp_VirtualAddress)];

    if (not (f_Pml4Entry & VMM_FLAG_PRESENT)) 
    { 
        return; 
    }

    uint64_t* f_Pdpt = Internal_PhysicalToVirtualAdress(f_Pml4Entry & VMM_PHYSICAL_ADDRESS_MASK);
    uint64_t  f_PdptEntry = f_Pdpt[PdptIndex(fp_VirtualAddress)];

    if (not (f_PdptEntry & VMM_FLAG_PRESENT)) 
    { 
        return; 
    }

    uint64_t* f_Pd = Internal_PhysicalToVirtualAdress(f_PdptEntry & VMM_PHYSICAL_ADDRESS_MASK);
    uint64_t  f_PdEntry = f_Pd[PdIndex(fp_VirtualAddress)];

    if (not (f_PdEntry & VMM_FLAG_PRESENT)) 
    { 
        return; 
    }

    uint64_t* f_Pt      = Internal_PhysicalToVirtualAdress(f_PdEntry & VMM_PHYSICAL_ADDRESS_MASK);
    uint64_t  f_PtEntry = f_Pt[PtIndex(fp_VirtualAddress)];

    if (not (f_PtEntry & VMM_FLAG_PRESENT)) 
    { 
        return; 
    }

    phys_addr_t f_PhysicalAddress = (phys_addr_t)(f_PtEntry & VMM_PHYSICAL_ADDRESS_MASK);

    /* clear the entry */
    f_Pt[PtIndex(fp_VirtualAddress)] = 0;

    /* flush TLB for this address */
    VioletArch_InvalidatePage(fp_VirtualAddress);

    if (fp_FreePhysical)
    {
        VioletPmm_FreePage(f_PhysicalAddress);
    }
}

/*============================================================
    VioletVmm_MapRange
==============================================================*/

bool 
    VioletVmm_MapRange
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtualBase,
        phys_addr_t             fp_PhysicalBase,
        size_t                  fp_PageCount,
        uint64_t                fp_Flags
    )
{
    for (size_t lv_Index = 0; lv_Index < fp_PageCount; lv_Index++)
    {
        virt_addr_t f_VirtAddr = fp_VirtualBase + lv_Index*VIOLET_PAGE_SIZE;
        phys_addr_t f_PhysAddr = fp_PhysicalBase + lv_Index*VIOLET_PAGE_SIZE;

        if (not VioletVmm_MapPage(fp_Space, f_VirtAddr, f_PhysAddr, fp_Flags))
        {
            VioletVmm_UnmapRange(fp_Space, fp_VirtualBase, lv_Index, false); /* partial failure; unmap what we mapped so far */
            
            return false;
        }
    }

    return true;
}

/*============================================================
    VioletVmm_UnmapRange
==============================================================*/

void 
    VioletVmm_UnmapRange
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtualBase,
        size_t                  fp_PageCount,
        bool                    fp_FreePhysical
    )
{
    for (size_t lv_Index = 0; lv_Index < fp_PageCount; lv_Index++)
    {
        VioletVmm_UnmapPage
        (
            fp_Space,
            fp_VirtualBase + lv_Index*VIOLET_PAGE_SIZE,
            fp_FreePhysical
        );
    }
}

/*============================================================
    VioletVmm_VirtToPhys
==============================================================*/

phys_addr_t 
    VioletVmm_VirtualToPhysicalAddress
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtAddr
    )
{
    if (fp_Space == nullptr) 
    { 
        return 0; 
    }

    uint64_t* f_Pml4      = Internal_PhysicalToVirtualAdress(fp_Space->Pml4PhysicalAddress);
    uint64_t  f_Pml4Entry = f_Pml4[Pml4Index(fp_VirtAddr)];

    if (not (f_Pml4Entry & VMM_FLAG_PRESENT)) 
    { 
        return 0; 
    }

    uint64_t* f_Pdpt      = Internal_PhysicalToVirtualAdress(f_Pml4Entry & VMM_PHYSICAL_ADDRESS_MASK);
    uint64_t  f_PdptEntry = f_Pdpt[PdptIndex(fp_VirtAddr)];

    if (not (f_PdptEntry & VMM_FLAG_PRESENT)) 
    { 
        return 0; 
    }

    uint64_t* f_Pd      = Internal_PhysicalToVirtualAdress(f_PdptEntry & VMM_PHYSICAL_ADDRESS_MASK);
    uint64_t  f_PdEntry = f_Pd[PdIndex(fp_VirtAddr)];

    if (not (f_PdEntry & VMM_FLAG_PRESENT)) 
    { 
        return 0; 
    }

    /* check for 2MB huge page — PD entry with PS bit set, no PT level */
    if (f_PdEntry & VMM_FLAG_HUGE)
    {
        phys_addr_t f_PageBase = (phys_addr_t)(f_PdEntry & 0x000FFFFFFFE00000ULL);

        return f_PageBase + (fp_VirtAddr & 0x1FFFFF);
    }

    uint64_t* f_Pt      = Internal_PhysicalToVirtualAdress(f_PdEntry & VMM_PHYSICAL_ADDRESS_MASK);
    uint64_t  f_PtEntry = f_Pt[PtIndex(fp_VirtAddr)];

    if (not (f_PtEntry & VMM_FLAG_PRESENT)) 
    { 
        return 0;
    }

    phys_addr_t f_PageBase = (phys_addr_t)(f_PtEntry & VMM_PHYSICAL_ADDRESS_MASK);

    return f_PageBase + (fp_VirtAddr & 0xFFF);
}

/*============================================================
    internal: find a free virtual range in an address space
    scans the region list for a gap large enough for fp_PageCount pages
    returns the base virtual address of the gap, 0 if none found
==============================================================*/

static virt_addr_t 
    FindFreeVirtualRange
    (
        VioletVmm_AddressSpace* fp_Space,
        size_t                  fp_PageCount,
        bool                    fp_IsKernel
    )
{
    virt_addr_t f_SearchStart = fp_IsKernel ? VIOLET_KERNEL_SPACE_START : VIOLET_USER_SPACE_START;
    virt_addr_t f_SearchEnd   = fp_IsKernel ? VIOLET_KERNEL_IMAGE_BASE  : VIOLET_USER_SPACE_END;
    size_t      f_NeededBytes = fp_PageCount * VIOLET_PAGE_SIZE;

    /*
        scan through existing regions sorted by base address
        find the first gap large enough for our allocation

        this is O(n^2) but for VMM_MAX_REGIONS_PER_SPACE = 256 that's fine
        upgrade to an interval tree or sorted list when we have dynamic allocation
    */
    virt_addr_t f_Candidate = f_SearchStart;

    for (;;)
    {
        /* check if f_Candidate is inside any existing region */
        bool     f_Overlaps   = false;
        virt_addr_t f_NextRegionBase = f_SearchEnd;

        for (size_t lv_Index = 0; lv_Index < VMM_MAX_REGIONS_PER_SPACE; lv_Index++)
        {
            VioletVmm_VmaRegion* f_Region = &fp_Space->Regions[lv_Index];

            if (not f_Region->IsAllocated)
            {
                continue;
            }

            virt_addr_t f_RegionEnd = f_Region->Base + f_Region->PageCount * VIOLET_PAGE_SIZE;

            /* does the candidate range overlap this region? */
            if (f_Candidate < f_RegionEnd and f_Candidate + f_NeededBytes > f_Region->Base)
            {
                f_Overlaps      = true;
                /* skip past this region and try again */
                f_Candidate     = f_RegionEnd;
                break;
            }

            /* track the start of the next region after our candidate */
            if (f_Region->Base > f_Candidate and f_Region->Base < f_NextRegionBase)
            {
                f_NextRegionBase = f_Region->Base;
            }
        }

        if (f_Overlaps)
        {
            /* page-align the next candidate */
            f_Candidate = (f_Candidate + (VIOLET_PAGE_SIZE - 1)) & ~(virt_addr_t)(VIOLET_PAGE_SIZE - 1);
            continue;
        }

        /* check if the gap between candidate and next region is large enough */
        if (f_Candidate + f_NeededBytes <= f_NextRegionBase)
        {
            return f_Candidate;
        }

        /* gap too small — skip past the next region */
        f_Candidate = f_NextRegionBase;

        if (f_Candidate >= f_SearchEnd)
        {
            return 0; /* no space found */
        }
    }
}

/*============================================================
    internal: add a region record to an address space
    returns pointer to the slot, nullptr if the pool is full
==============================================================*/

static VioletVmm_VmaRegion* 
    Internal_AddRegion
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_Base,
        size_t                  fp_PageCount,
        uint64_t                fp_Flags
    )
{
    for (size_t lv_Index = 0; lv_Index < VMM_MAX_REGIONS_PER_SPACE; lv_Index++)
    {
        VioletVmm_VmaRegion* f_Region = &fp_Space->Regions[lv_Index];

        if (not f_Region->IsAllocated)
        {
            f_Region->Base        = fp_Base;
            f_Region->PageCount   = fp_PageCount;
            f_Region->Flags       = fp_Flags;
            f_Region->IsAllocated = true;
            fp_Space->RegionCount++;
            
            return f_Region;
        }
    }

    return nullptr; /* pool full */
}

/*============================================================
    internal: find and remove a region record by base address
==============================================================*/

// static void 
//     RemoveRegion
//     (
//         VioletVmm_AddressSpace* fp_Space,
//         virt_addr_t             fp_Base
//     )
// {
//     for (size_t lv_Index = 0; lv_Index < VMM_MAX_REGIONS_PER_SPACE; lv_Index++)
//     {
//         VioletVmm_VmaRegion* f_Region = &fp_Space->Regions[lv_Index];

//         if (f_Region->IsAllocated and f_Region->Base == fp_Base)
//         {
//             f_Region->IsAllocated = false;
//             fp_Space->RegionCount--;
//             return;
//         }
//     }
// }

/*============================================================
    VioletVmm_AllocatePages
==============================================================*/

virt_addr_t 
    VioletVmm_AllocatePages
    (
        VioletVmm_AddressSpace* fp_Space,
        size_t                  fp_PageCount,
        uint64_t                fp_Flags
    )
{
    VIOLET_PANIC_IF(fp_Space == nullptr, "nullptr reference to address space was passed ;w;");
    VIOLET_PANIC_IF(fp_PageCount == 0, "tried to allocate 0 pages");

    virt_addr_t f_VirtBase = FindFreeVirtualRange(fp_Space, fp_PageCount, fp_Space->IsKernel);

    if (f_VirtBase == 0)
    {
        return 0; /* out of virtual address space */
    }

    /* allocate and map each page */
    for (size_t lv_Index = 0; lv_Index < fp_PageCount; lv_Index++)
    {
        phys_addr_t f_PhysPage = VioletPmm_AllocatePage();

        if (f_PhysPage == 0)
        {
            /* out of physical memory — unmap and free what we did so far */
            VioletVmm_UnmapRange(fp_Space, f_VirtBase, lv_Index, true);

            return 0;
        }

        virt_addr_t f_VirtPage = f_VirtBase + lv_Index * VIOLET_PAGE_SIZE;

        if (not VioletVmm_MapPage(fp_Space, f_VirtPage, f_PhysPage, fp_Flags))
        {
            VioletPmm_FreePage(f_PhysPage);
            VioletVmm_UnmapRange(fp_Space, f_VirtBase, lv_Index, true);

            return 0;
        }
    }

    VIOLET_PANIC_IF(Internal_AddRegion(fp_Space, f_VirtBase, fp_PageCount, fp_Flags) == nullptr, "VMM region pool exhausted — increase VMM_MAX_REGIONS_PER_SPACE");

    return f_VirtBase;
}

/*============================================================
    VioletVmm_FreePages
==============================================================*/

void 
    VioletVmm_FreePages
    (
        VioletVmm_AddressSpace* fp_Space,
        virt_addr_t             fp_VirtBase
    )
{
    if (fp_Space == nullptr)
    {
        return;
    }

    /* find the region record to get the page count */
    for (size_t lv_Index = 0; lv_Index < VMM_MAX_REGIONS_PER_SPACE; lv_Index++)
    {
        VioletVmm_VmaRegion* f_Region = &fp_Space->Regions[lv_Index];

        if (f_Region->IsAllocated and f_Region->Base == fp_VirtBase)
        {
            VioletVmm_UnmapRange(fp_Space, fp_VirtBase, f_Region->PageCount, true);
            f_Region->IsAllocated = false;
            fp_Space->RegionCount--;

            return;
        }
    }

    /* address wasn't tracked — either a bug or already freed */
    VIOLET_PANIC_IF(true, "VioletVmm_FreePages: address was not allocated via VioletVmm_AllocatePages");
}

/*============================================================
    VioletVmm_CreateAddressSpace
==============================================================*/

VioletVmm_AddressSpace* 
    VioletVmm_CreateAddressSpace()
{
    /* find a free slot in the pool */
    VioletVmm_AddressSpace* f_Space = nullptr;

    for (size_t lv_Index = 0; lv_Index < VMM_MAX_ADDRESS_SPACES; lv_Index++)
    {
        if (not s_AddressSpacePoolUsed[lv_Index])
        {
            s_AddressSpacePoolUsed[lv_Index] = true;
            f_Space = &s_AddressSpacePool[lv_Index];
            break;
        }
    }

    if (f_Space == nullptr)
    {
        return nullptr; /* pool full */
    }

    /* zero the structure */
    memset_as_u8(f_Space, 0, sizeof(VioletVmm_AddressSpace));

    /* allocate a new PML4 page */
    phys_addr_t f_Pml4 = VioletPmm_AllocatePage();

    if (f_Pml4 == 0)
    {
        s_AddressSpacePoolUsed[f_Space - s_AddressSpacePool] = false;
        return nullptr;
    }

    f_Space->Pml4PhysicalAddress = f_Pml4;
    f_Space->IsKernel     = false;

    /*
        copy kernel PML4 entries into the new space's PML4
        the upper 256 entries (indices 256-511) are the kernel half
        sharing them means every address space automatically sees
        kernel mappings without any per-space synchronisation
        this is the standard higher-half kernel design
    */
    uint64_t* f_NewPml4    = Internal_PhysicalToVirtualAdress(f_Pml4);
    uint64_t* f_KernelPml4 = Internal_PhysicalToVirtualAdress(s_KernelAddressSpace.Pml4PhysicalAddress);

    for (size_t lv_Index = 256; lv_Index < 512; lv_Index++)
    {
        f_NewPml4[lv_Index] = f_KernelPml4[lv_Index];
    }

    return f_Space;
}

/*============================================================
    VioletVmm_DestroyAddressSpace
==============================================================*/

void 
    VioletVmm_DestroyAddressSpace(VioletVmm_AddressSpace* fp_Space)
{
    VIOLET_PANIC_IF(fp_Space == nullptr, "[INTERNAL ERROR]: passed nullptr reference to vmm address space ;w;");
    VIOLET_PANIC_IF(fp_Space->IsKernel,  "[INTERNAL ERROR]: tried to destroy kernel space!");

    /* free all tracked user-space allocations */
    for (size_t lv_Index = 0; lv_Index < VMM_MAX_REGIONS_PER_SPACE; lv_Index++)
    {
        VioletVmm_VmaRegion* f_Region = &fp_Space->Regions[lv_Index];

        if (f_Region->IsAllocated)
        {
            VioletVmm_UnmapRange(fp_Space, f_Region->Base, f_Region->PageCount, true);
            f_Region->IsAllocated = false;
        }
    }

    /*
        free the page table pages themselves (PTs, PDs, PDPTs)
        walk only the lower half (indices 0-255) — the upper half
        is shared kernel mappings we must not free
    */
    uint64_t* f_Pml4 = Internal_PhysicalToVirtualAdress(fp_Space->Pml4PhysicalAddress);

    for (size_t lv_Pml4Idx = 0; lv_Pml4Idx < 256; lv_Pml4Idx++)
    {
        if (not (f_Pml4[lv_Pml4Idx] & VMM_FLAG_PRESENT)) 
        { 
            continue; 
        }

        uint64_t* f_Pdpt     = Internal_PhysicalToVirtualAdress(f_Pml4[lv_Pml4Idx] & VMM_PHYSICAL_ADDRESS_MASK);
        phys_addr_t f_PdptPhys = (phys_addr_t)(f_Pml4[lv_Pml4Idx] & VMM_PHYSICAL_ADDRESS_MASK);

        for (size_t lv_PdptIdx = 0; lv_PdptIdx < 512; lv_PdptIdx++)
        {
            if (not (f_Pdpt[lv_PdptIdx] & VMM_FLAG_PRESENT)) 
            { 
                continue; 
            }

            uint64_t* f_Pd       = Internal_PhysicalToVirtualAdress(f_Pdpt[lv_PdptIdx] & VMM_PHYSICAL_ADDRESS_MASK);
            phys_addr_t f_PdPhys = (phys_addr_t)(f_Pdpt[lv_PdptIdx] & VMM_PHYSICAL_ADDRESS_MASK);

            for (size_t lv_PdIdx = 0; lv_PdIdx < 512; lv_PdIdx++)
            {
                if (not (f_Pd[lv_PdIdx] & VMM_FLAG_PRESENT)) 
                { 
                    continue; 
                }

                if (f_Pd[lv_PdIdx] & VMM_FLAG_HUGE) 
                { 
                    continue; /* huge page, no PT to free */ 
                }

                VioletPmm_FreePage((phys_addr_t)(f_Pd[lv_PdIdx] & VMM_PHYSICAL_ADDRESS_MASK));
            }

            VioletPmm_FreePage(f_PdPhys);
        }

        VioletPmm_FreePage(f_PdptPhys);
    }

    /* free the PML4 itself */
    VioletPmm_FreePage(fp_Space->Pml4PhysicalAddress);

    /* return the pool slot */
    size_t f_SlotIndex = (size_t)(fp_Space - s_AddressSpacePool);
    s_AddressSpacePoolUsed[f_SlotIndex] = false;
}

/*============================================================
    VioletVmm_SwitchAddressSpace
==============================================================*/

void 
    VioletVmm_SwitchAddressSpace(const VioletVmm_AddressSpace* fp_Space)
{
    VIOLET_PANIC_IF(fp_Space == nullptr, "VioletVmm_SwitchAddressSpace: null space");

    VioletArch_LoadPageTable(fp_Space->Pml4PhysicalAddress);
}

/*============================================================
    VioletVmm_GetKernelSpace
==============================================================*/

VioletVmm_AddressSpace* 
    VioletVmm_GetKernelSpace(void)
{
    return &s_KernelAddressSpace;
}

/*============================================================
    VioletVmm_Init
==============================================================*/

void 
    VioletVmm_Init
    (
        const VioletBoot_Info* fp_BootInfo
    )
{
    VIOLET_PANIC_IF(fp_BootInfo == nullptr, "nullptr reference to boot info was passed ;w; water fuck man");

    s_KernelAddressSpace.Pml4PhysicalAddress = VioletArch_ReadPageTable(); // take ownership of the bootloader's PML4 since the direct map was already built in the bootloader uwu
    s_KernelAddressSpace.IsKernel    = true;
    s_KernelAddressSpace.RegionCount = 0;

    memset_as_u8(s_KernelAddressSpace.Regions, 0, sizeof(s_KernelAddressSpace.Regions));
}