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
#ifndef VIOLET_BOOTLOADER_MEMORY_MAP_HG
#define VIOLET_BOOTLOADER_MEMORY_MAP_HG

#include <Uefi.h>

typedef struct {
    EFI_MEMORY_DESCRIPTOR* Descriptors;     // pointer to the descriptor array
    UINTN                  MapSize;         // total byte size of the descriptor array
    UINTN                  DescriptorCount; // MapSize / DescriptorSize
    UINTN                  MapKey;          // needed for ExitBootServices()
    UINTN                  DescriptorSize;  // size of ONE descriptor, NOT sizeof(EFI_MEMORY_DESCRIPTOR)! UEFI can extend this struct, always use the returned size to walk the array
    UINT32                 DescriptorVersion; // >w<
} VioletMemoryMap;


UINTN 
    VioletMemoryMap_CountUsablePages(const VioletMemoryMap* fp_Map);

VioletMemoryMap 
    VioletMemoryMap_CreateEmpty(void);

#endif /*VIOLET_BOOTLOADER_MEMORY_MAP_HG*/

