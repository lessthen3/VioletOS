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
#include "PhysicalMemoryManager.h"

///VioletShared
#include "shared/GeneralMacros.h"

///VioletKernel
#include "kernel/VioletPanic.h"
#include "kernel/vklib/memset.h"

///CSTD
#include <iso646.h>

/*============================================================
    UEFI memory type constants
    mirrored here so we don't need UEFI headers in the kernel
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
    global PMM state — lives in BSS, zeroed before KernelMain runs
==============================================================*/

static Violet_Pmm singleton_Pmm;

#define BITS_PER_LONG (sizeof(size_t) * 8)
#define BITMAP_SHIFT_DIVISION  (sizeof(size_t) == 8 ? 6 : 5)
#define BITMAP_MASK_MODULO   (BITS_PER_LONG - 1)

/*============================================================
    internal bitmap helpers
==============================================================*/

/*
    We're always dividing by 64 or 32 which are powers of 2, which is equivalent to a bitshift
*/

VIOLET_FORCEINLINE static void 
    Internal_BitmapSetBit(size_t* fp_Bitmap, size_t fp_BitIndex)
{
    fp_Bitmap[fp_BitIndex >> BITMAP_SHIFT_DIVISION] |= ((size_t)1 << (fp_BitIndex & BITMAP_MASK_MODULO));
}

VIOLET_FORCEINLINE static void 
    Internal_BitmapClearBit(size_t* fp_Bitmap, size_t fp_BitIndex)
{
    fp_Bitmap[fp_BitIndex >> BITMAP_SHIFT_DIVISION] &= ~((size_t)1 << (fp_BitIndex & BITMAP_MASK_MODULO));
}

VIOLET_FORCEINLINE static int 
    Internal_BitmapTestBit(const size_t* fp_Bitmap, size_t fp_BitIndex)
{
    return (fp_Bitmap[fp_BitIndex >> BITMAP_SHIFT_DIVISION] >> (fp_BitIndex & BITMAP_MASK_MODULO)) & 1;
}

/*============================================================
    internal helpers for marking page ranges
==============================================================*/

VIOLET_FORCEINLINE static void 
    Internal_MarkPagesUsed
    (
        size_t fp_PhysicalBase, 
        size_t fp_PageCount
    )
{
    size_t f_StartPage = fp_PhysicalBase / VIOLET_PAGE_SIZE;

    for (size_t lv_Index = 0; lv_Index < fp_PageCount; lv_Index++)
    {
        size_t f_Page = f_StartPage + lv_Index;

        if (f_Page < singleton_Pmm.TotalPages and not Internal_BitmapTestBit(singleton_Pmm.Bitmap, f_Page))
        {
            Internal_BitmapSetBit(singleton_Pmm.Bitmap, f_Page);
            singleton_Pmm.FreePages--;
        }
    }
}

VIOLET_FORCEINLINE static void 
    Internal_MarkPagesFree
    (
        size_t fp_PhysicalBase, 
        size_t fp_PageCount
    )
{
    size_t f_StartPage = fp_PhysicalBase / VIOLET_PAGE_SIZE;

    for (size_t lv_Index = 0; lv_Index < fp_PageCount; lv_Index++)
    {
        size_t f_Page = f_StartPage + lv_Index;

        if (f_Page < singleton_Pmm.TotalPages and Internal_BitmapTestBit(singleton_Pmm.Bitmap, f_Page))
        {
            Internal_BitmapClearBit(singleton_Pmm.Bitmap, f_Page);
            singleton_Pmm.FreePages++;
        }
    }
}

/*============================================================
    VioletPmm_Init
==============================================================*/

void 
    VioletPmm_Init
    (
        const VioletBoot_Info* fp_BootInfo,
        size_t                 fp_KernelPhysicalEnd
    )
{
    singleton_Pmm.Bitmap          = (size_t*)fp_BootInfo->BitmapPhysicalAddress;
    singleton_Pmm.TotalPages      = fp_BootInfo->TotalPageCount;
    singleton_Pmm.FreePages       = 0;
    singleton_Pmm.LastSearchIndex = 0;

    //////////////////////////////////////// initialise bitmap to all 1s (all pages used) ////////////////////////////////////////

    /*
        we do this so we can then selectively free only the regions we know are safe;
        this is much safer than assuming all of memory is free and marking whats used; 
        we avoid accidentally freeing firmware memory we missed, or some random firmware bugs (rare but can happen owo)
    */

    memset_as_u8(singleton_Pmm.Bitmap, 0xFF, fp_BootInfo->BitmapSizeBytes);

    //////////////////////////////////////// free all usable conventional memory ////////////////////////////////////////
    /*
        
        EfiConventionalMemory is free right now
        EfiBootServicesCode + EfiBootServicesData are free because ExitBootServices was called
        everything else stays marked used
    */

    const VioletBoot_MemoryMap* f_Map = &(fp_BootInfo->MemoryMap);

    for (size_t lv_Index = 0; lv_Index < f_Map->DescriptorCount; lv_Index++)
    {
        VioletBoot_MemoryDescriptor* f_Desc = (VioletBoot_MemoryDescriptor*)((uint8_t*)f_Map->Descriptors + lv_Index*f_Map->DescriptorSize);

        // have to preserve memory marked as EFI_RUNTIME_SERVICES_CODE and EFI_RUNTIME_SERVICES_DATA as per the spec 2.11 (section 7.2 table 7.6)

        if (f_Desc->Type == EFI_CONVENTIONAL_MEMORY or f_Desc->Type == EFI_BOOT_SERVICES_CODE or f_Desc->Type == EFI_BOOT_SERVICES_DATA)
        {
            Internal_MarkPagesFree(f_Desc->PhysicalStart, f_Desc->NumberOfPages);
        }
    }

    //////////////////////////////////////// now re-mark regions that must stay reserved even though they're in conventional memory ranges or boot services memory ////////////////////////////////////////

    /*
        kernel image; 
        it's literally the code we're running right now, fairly self explanatory imo
    */

    size_t f_KernelPages = (fp_KernelPhysicalEnd - 0x200000ULL + VIOLET_PAGE_SIZE - 1) / VIOLET_PAGE_SIZE;
    Internal_MarkPagesUsed(0x200000ULL, f_KernelPages);

    /* 
        the bitmap itself 
    */

    size_t f_BitmapPages = (fp_BootInfo->BitmapSizeBytes + VIOLET_PAGE_SIZE - 1) / VIOLET_PAGE_SIZE;
    Internal_MarkPagesUsed((size_t)singleton_Pmm.Bitmap, f_BitmapPages);

    /* 
        memory map descriptor array; still being referenced 
    */

    size_t f_MapBytes = f_Map->DescriptorCount * f_Map->DescriptorSize;
    size_t f_MapPages = (f_MapBytes + VIOLET_PAGE_SIZE - 1) / VIOLET_PAGE_SIZE;
    Internal_MarkPagesUsed((size_t)f_Map->Descriptors, f_MapPages);

    /// these ones aren't required since we marked them propery so our loop above doesn't mark them as free but can still do it
    /// this method is only called once ever so ye uwu

    /* 
        framebuffer MMIO; mark used so PMM never hands it out as RAM 
    */

    size_t f_FrameBufferBytes = (size_t)fp_BootInfo->FrameBuffer.Height * fp_BootInfo->FrameBuffer.PixelsPerScanLine * 4;
    size_t f_FrameBufferPages = (f_FrameBufferBytes + VIOLET_PAGE_SIZE - 1) / VIOLET_PAGE_SIZE;
    Internal_MarkPagesUsed(fp_BootInfo->FrameBuffer.FrameBufferBase, f_FrameBufferPages);

    /*
        The boot info struct itself
    */

    size_t f_BootInfoPages = (sizeof(VioletBoot_Info) + VIOLET_PAGE_SIZE - 1) / VIOLET_PAGE_SIZE;
    Internal_MarkPagesUsed((size_t)fp_BootInfo, f_BootInfoPages);

    /* 
        first 1MB is a legacy BIOS/UEFI reserved region, NEVER EVER EVER EVER TOUCH THIS 
    */
    Internal_MarkPagesUsed(0x0, 256);
}

/*============================================================
    VioletPmm_AllocatePage
==============================================================*/

size_t 
    VioletPmm_AllocatePage()
{
    /*
        search for a free page starting from LastSearchIndex
        wraps around once if we reach the end without finding one
    */
    for (size_t lv_Pass = 0; lv_Pass < 2; lv_Pass++)
    {
        for (size_t lv_Index = singleton_Pmm.LastSearchIndex; lv_Index < singleton_Pmm.TotalPages; lv_Index++)
        {
            if (not Internal_BitmapTestBit(singleton_Pmm.Bitmap, lv_Index))
            {
                Internal_BitmapSetBit(singleton_Pmm.Bitmap, lv_Index);
                
                singleton_Pmm.FreePages--;
                singleton_Pmm.LastSearchIndex = lv_Index + 1;

                /* zero the page before handing it out */
                memset_as_u8((void*)(lv_Index * VIOLET_PAGE_SIZE), 0, VIOLET_PAGE_SIZE);

                return lv_Index * VIOLET_PAGE_SIZE;
            }
        }

        /* wrap around and search from the beginning */
        singleton_Pmm.LastSearchIndex = 0;
    }

    return 0; /* out of memory */
}

/*============================================================
    VioletPmm_AllocatePages
==============================================================*/

size_t 
    VioletPmm_AllocatePages(size_t fp_Count)
{
    if (fp_Count == 0)
    {
        return 0;
    }

    if (fp_Count == 1)
    {
        return VioletPmm_AllocatePage();
    }

    /*
        find fp_Count contiguous free pages
        scan for a run of fp_Count consecutive 0 bits
    */
    size_t f_RunStart  = 0;
    size_t f_RunLength = 0;

    for (size_t lv_Index = 0; lv_Index < singleton_Pmm.TotalPages; lv_Index++)
    {
        if (not Internal_BitmapTestBit(singleton_Pmm.Bitmap, lv_Index))
        {
            if (f_RunLength == 0)
            {
                f_RunStart = lv_Index;
            }

            f_RunLength++;

            if (f_RunLength == fp_Count)
            {
                /* found a run — mark all pages used */
                for (size_t lv_Page = f_RunStart; lv_Page < f_RunStart + fp_Count; lv_Page++)
                {
                    Internal_BitmapSetBit(singleton_Pmm.Bitmap, lv_Page);
                }

                singleton_Pmm.FreePages -= fp_Count;

                return f_RunStart * VIOLET_PAGE_SIZE;
            }
        }
        else
        {
            /* run broken — reset */
            f_RunLength = 0;
        }
    }

    return 0; /* no contiguous region found */
}

/*============================================================
    VioletPmm_FreePage
==============================================================*/

void 
    VioletPmm_FreePage(size_t fp_PhysicalAddress)
{
    size_t f_PageIndex = fp_PhysicalAddress / VIOLET_PAGE_SIZE;

    if (f_PageIndex >= singleton_Pmm.TotalPages)
    {
        return;
    }

    VIOLET_PANIC_IF(not Internal_BitmapTestBit(singleton_Pmm.Bitmap, f_PageIndex), "[PMM ERROR]: tried to free a memory page that was already freed");

    Internal_BitmapClearBit(singleton_Pmm.Bitmap, f_PageIndex);
    singleton_Pmm.FreePages++;

    /* hint the next search to start here — good for free-then-reallocate patterns */
    if (f_PageIndex < singleton_Pmm.LastSearchIndex)
    {
        singleton_Pmm.LastSearchIndex = f_PageIndex;
    }
}

/*============================================================
    VioletPmm_FreePages
==============================================================*/

void 
    VioletPmm_FreePages(size_t fp_PhysicalAddress, size_t fp_Count)
{
    for (size_t lv_Index = 0; lv_Index < fp_Count; lv_Index++)
    {
        VioletPmm_FreePage(fp_PhysicalAddress + lv_Index*VIOLET_PAGE_SIZE);
    }
}

/*============================================================
    VioletPmm_GetFreePageCount
==============================================================*/

size_t 
    VioletPmm_GetFreePageCount()
{
    return singleton_Pmm.FreePages;
}

/*============================================================
    VioletPmm_GetTotalPageCount
==============================================================*/

size_t 
    VioletPmm_GetTotalPageCount()
{
    return singleton_Pmm.TotalPages;
}