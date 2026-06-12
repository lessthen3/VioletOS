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
#ifndef VIOLET_SHARED_BOOT_INFO_HG
#define VIOLET_SHARED_BOOT_INFO_HG

#include <stdint.h>

///VioletShared
#include "gop/GopFrameBuffer.h"

/* 
    This is just mirroring the EFI_MEMORY_DESCRIPTOR type from UefiSpec.h so that I don't need to
    throw UEFI headers in the kernel uwu owo
*/

typedef struct {
    uint32_t Type;

    uint64_t PhysicalStart;
    uint64_t VirtualStart;

    uint64_t NumberOfPages;

    uint64_t Attribute;                
} VioletBoot_MemoryDescriptor;

typedef struct {
    uint8_t* Descriptors;        /* array of descriptors */

    uint64_t                     MapSize;   
    uint64_t                     DescriptorCount;   
    uint64_t                     MapKey;    

    uint64_t                     DescriptorSize;  // size of ONE descriptor, NOT sizeof(EFI_MEMORY_DESCRIPTOR)! UEFI can extend this struct, always use the returned size to walk the array
    uint32_t                     DescriptorVersion; // >w<
} VioletBoot_MemoryMap;

typedef struct {
    VioletGop_FrameBuffer   FrameBuffer;
    VioletBoot_MemoryMap    MemoryMap;

    uint64_t                BitmapPhysicalAddress; // where bootloader mapped pages for the bitmap
    uint64_t                BitmapSizeBytes;       // how big it is

    uint64_t                TotalPageCount;

    uint64_t BootloaderCodeBase;
    uint64_t BootloaderCodePageAmount;
    uint64_t BootloaderStackBase;
    uint64_t BootloaderStackPageAmount;
} VioletBoot_Info;

#endif /*VIOLET_SHARED_BOOT_INFO_HG*/