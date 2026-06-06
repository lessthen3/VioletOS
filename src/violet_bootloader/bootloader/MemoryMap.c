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
#include "MemoryMap.h"
#include <iso646.h>

VioletMemoryMap 
    VioletMemoryMap_CreateEmpty(void)
{
    VioletMemoryMap f_MemoryMap;

    f_MemoryMap.Descriptors = NULL;
    f_MemoryMap.MapSize = 0;
    f_MemoryMap.DescriptorCount = 0;
    f_MemoryMap.MapKey = 0;
    f_MemoryMap.DescriptorSize = 0;
    f_MemoryMap.DescriptorVersion = 0;

    return f_MemoryMap;
}

UINTN 
    VioletMemoryMap_CountUsablePages(const VioletMemoryMap* fp_MemoryMap)
{
    if(not fp_MemoryMap)
    {

        return 0;
    }

    UINTN f_TotalPages = 0;

    for (UINTN lv_Index = 0; lv_Index < fp_MemoryMap->DescriptorCount; lv_Index++)
    {
        /*
            must use DescriptorSize to walk; 
            firmware can extend the struct beyond what the spec defines;
            indexing as a normal array would land on garbage after the first entry
        */
        EFI_MEMORY_DESCRIPTOR* f_Descriptor = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)fp_MemoryMap->Descriptors + lv_Index*fp_MemoryMap->DescriptorSize);

        /*
            EfiConventionalMemory  — free RAM right now
            EfiBootServicesCode    — UEFI boot service code, freed after ExitBootServices
            EfiBootServicesData    — UEFI boot service data, freed after ExitBootServices
            everything else is either reserved forever or MMIO
        */
        if (f_Descriptor->Type == EfiConventionalMemory or f_Descriptor->Type == EfiBootServicesCode or f_Descriptor->Type == EfiBootServicesData)
        {
            f_TotalPages += f_Descriptor->NumberOfPages;
        }
    }

    return f_TotalPages;
}