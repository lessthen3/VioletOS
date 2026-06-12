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
#include "PageTable.h"

///VioletShared
#include "shared/arch/MemoryWidthTypes.h"

///CSTD
#include <iso646.h>

/*
    allocate one zeroed 4KB page using UEFI boot services
    returns physical address of the page, 0 on failure
    AllocatePages returns 4KB-aligned pages by definition
*/
static uint64_t 
    Internal_AllocateZeroedPage(EFI_SYSTEM_TABLE* fp_SystemTable)
{
    EFI_PHYSICAL_ADDRESS f_Page = 0;

    EFI_STATUS f_Status = fp_SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &f_Page);

    if (EFI_ERROR(f_Status))
    {
        return 0;
    }

    /*
        actually zero the page, page table entries must be 0 (not present)
        by default, any garbage left here would be interpreted as valid mappings
    */
    uint64_t* f_Entries = (uint64_t*)f_Page;

    for (int lv_Index = 0; lv_Index < 512; lv_Index++)
    {
        f_Entries[lv_Index] = 0;
    }

    return (uint64_t)f_Page;
}

/*
    extract the index into a page table level from a virtual address
    each index is 9 bits, positioned differently per level
*/
static uint64_t 
    Internal_Pml4Index(uint64_t fp_VirtualAddress) 
{ 
    return (fp_VirtualAddress >> 39) & 0x1FF; 
}

static uint64_t 
    Internal_PdptIndex(uint64_t fp_VirtualAddress) 
{ 
    return (fp_VirtualAddress >> 30) & 0x1FF; 
}

static uint64_t 
    Internal_PdIndex(uint64_t fp_VirtualAddress) 
{ 
    return (fp_VirtualAddress >> 21) & 0x1FF; 
}

static uint64_t 
    Internal_PtIndex(uint64_t fp_VirtualAddress) 
{ 
    return (fp_VirtualAddress >> 12) & 0x1FF; 
}

/*
    map a single 4KB page: virtual address → physical address
    creates intermediate tables as needed
    fp_Pml4: physical address of the root PML4 table
*/
EFI_STATUS 
    Violet_MapPage
    (
        EFI_SYSTEM_TABLE* fp_SystemTable,
        uint64_t          fp_Pml4,
        uint64_t          fp_VirtualAddress,
        uint64_t          fp_PhysicalAddress,
        uint64_t          fp_Flags
    )
{
    uint64_t* f_Pml4Table = (uint64_t*)fp_Pml4;
    uint64_t  f_Pml4Idx   = Internal_Pml4Index(fp_VirtualAddress);

    /* get or create PDPT */
    if (not (f_Pml4Table[f_Pml4Idx] & PAGE_TABLE_ENTRY_PRESENT))
    {
        uint64_t f_NewPdpt = Internal_AllocateZeroedPage(fp_SystemTable);

        if (f_NewPdpt == 0)
        {
            return EFI_OUT_OF_RESOURCES;
        }

        f_Pml4Table[f_Pml4Idx] = f_NewPdpt | PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE;
    }

    uint64_t* f_PdptTable = (uint64_t*)(f_Pml4Table[f_Pml4Idx] & ~(VIOLET_PAGE_SIZE - 1));
    uint64_t  f_PdptIdx   = Internal_PdptIndex(fp_VirtualAddress);

    /* get or create PD */
    if (not (f_PdptTable[f_PdptIdx] & PAGE_TABLE_ENTRY_PRESENT))
    {
        uint64_t f_NewPd = Internal_AllocateZeroedPage(fp_SystemTable);

        if (f_NewPd == 0)
        {
            return EFI_OUT_OF_RESOURCES;
        }

        f_PdptTable[f_PdptIdx] = f_NewPd | PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE;
    }

    uint64_t* f_PdTable = (uint64_t*)(f_PdptTable[f_PdptIdx] & ~(VIOLET_PAGE_SIZE - 1));
    uint64_t  f_PdIdx   = Internal_PdIndex(fp_VirtualAddress);

    /* get or create PT */
    if (not (f_PdTable[f_PdIdx] & PAGE_TABLE_ENTRY_PRESENT))
    {
        uint64_t f_NewPt = Internal_AllocateZeroedPage(fp_SystemTable);

        if (f_NewPt == 0)
        {
            return EFI_OUT_OF_RESOURCES;
        }

        f_PdTable[f_PdIdx] = f_NewPt | PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE;
    }

    uint64_t* f_PtTable = (uint64_t*)(f_PdTable[f_PdIdx] & ~(VIOLET_PAGE_SIZE - 1));
    uint64_t  f_PtIdx   = Internal_PtIndex(fp_VirtualAddress);

    /* write the final mapping */
    f_PtTable[f_PtIdx] = fp_PhysicalAddress | fp_Flags;

    return EFI_SUCCESS;
}

/*============================================================
    Violet_SetupPageTables
==============================================================*/

uint64_t 
    Violet_SetupPageTables
    (
        EFI_SYSTEM_TABLE* fp_SystemTable,
        uint64_t          fp_KernelPhysicalBase,
        uint64_t          fp_KernelVirtualBase,
        uint64_t          fp_KernelPageCount
    )
{
    /* allocate the root PML4 table */
    uint64_t f_Pml4 = Internal_AllocateZeroedPage(fp_SystemTable);

    if (f_Pml4 == 0)
    {
        return 0;
    }

    /*
        map each kernel page: VMA + offset → physical + offset
        .text gets PAGE_TABLE_ENTRY_PRESENT (no PAGE_TABLE_ENTRY_WRITABLE, no PAGE_TABLE_ENTRY_NO_EXECUTE so it's executable read-only)
        for simplicity we map everything writable+executable here
        the kernel's VMM will rebuild proper permission maps later
    */
    for (uint64_t lv_Page = 0; lv_Page < fp_KernelPageCount; lv_Page++)
    {
        uint64_t fv_VirtualAddress = fp_KernelVirtualBase + lv_Page*VIOLET_PAGE_SIZE;
        uint64_t fv_PhysicalAddress = fp_KernelPhysicalBase + lv_Page*VIOLET_PAGE_SIZE;

        EFI_STATUS fv_Status = Violet_MapPage
        (
            fp_SystemTable,
            f_Pml4,
            fv_VirtualAddress, fv_PhysicalAddress,
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE
        );

        if (EFI_ERROR(fv_Status))
        {
            return 0;
        }
    }

    /*
        identity map the physical region too
        needed because when we do MOV CR3, RAX the CPU switches tables immediately
        the very next instruction fetch uses the new tables
        if the bootloader's current RIP isn't mapped, instant triple fault
        this map keeps the bootloader code accessible for the few instructions
        between loading CR3 and jumping to the kernel VMA
    */
    for (uint64_t lv_Page = 0; lv_Page < fp_KernelPageCount; lv_Page++)
    {
        uint64_t fv_PhysicalAddress = fp_KernelPhysicalBase + lv_Page*VIOLET_PAGE_SIZE;

        EFI_STATUS fv_Status = Violet_MapPage
        (
            fp_SystemTable,
            f_Pml4,
            fv_PhysicalAddress, fv_PhysicalAddress,  //identity mapped uwu
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE
        );

        if (EFI_ERROR(fv_Status))
        {
            return 0;
        }
    }

    return f_Pml4;
}