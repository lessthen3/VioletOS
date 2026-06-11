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
#include "ElfLoader.h"
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <iso646.h>

/*============================================================
    internal helpers 
==============================================================*/

// static void
//     VioletMemcpy(void* fp_Destination, const void* fp_Source, uint64_t fp_Size)
// {
//     uint8_t*       f_Dst = (uint8_t*)fp_Destination;
//     const uint8_t* f_Src = (const uint8_t*)fp_Source;

//     for (uint64_t lv_Index = 0; lv_Index < fp_Size; lv_Index++)
//     {
//         f_Dst[lv_Index] = f_Src[lv_Index];
//     }
// }

static void
    Internal_Memset(void* fp_Destination, uint8_t fp_Value, uint64_t fp_Size)
{
    uint8_t* f_Dst = (uint8_t*)fp_Destination;

    for (uint64_t lv_Index = 0; lv_Index < fp_Size; lv_Index++)
    {
        f_Dst[lv_Index] = fp_Value;
    }
}

static int
    Internal_ValidateElfHeader(const Elf64Header* fp_Header)
{
    if(fp_Header->Ident[0] != ELF_MAGIC_0 or fp_Header->Ident[1] != ELF_MAGIC_1 or fp_Header->Ident[2] != ELF_MAGIC_2 or fp_Header->Ident[3] != ELF_MAGIC_3)
    {
        return 0; /* not an ELF file */
    }

    if(fp_Header->Ident[4] != ELF_CLASS_64)
    {
        return 0; /* not 64-bit */
    }

    if(fp_Header->Ident[5] != ELF_DATA_LSB)
    {
        return 0; /* not little-endian */
    }

    if(fp_Header->Machine != ELF_MACHINE_X86_64)
    {
        return 0; /* not x86-64 */
    }

    if(fp_Header->Type != ELF_TYPE_EXEC)
    {
        return 0; /* not an executable — kernel must be ET_EXEC not ET_DYN */
    }

    return 1;
}

/*============================================================
    Violet_LoadKernelElf
==============================================================*/

EFI_STATUS
    Violet_LoadKernelElf
    (
        EFI_HANDLE          fp_ImageHandle,
        EFI_SYSTEM_TABLE*   fp_SystemTable,
        VioletLoadedKernel* fp_OutKernel,
        VioletGop_Console*     fp_VioletConsole
    )
{
    if(not fp_VioletConsole)
    {
        return 69;
    }

    if(not fp_SystemTable)
    {

    }

    if(not fp_OutKernel)
    {

    }
    
    EFI_STATUS f_Status;

    /*
        ----------------------------------------------------------------
            get the filesystem the bootloader itself loaded from
        ----------------------------------------------------------------
        EFI_LOADED_IMAGE_PROTOCOL tells us which device we booted from;
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL on that device gives us filesystem access;
        this way we always read violet_kernel.elf from the same ESP we booted from;
        no hardcoded device handles
    */

    EFI_LOADED_IMAGE_PROTOCOL*       f_LoadedImage = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* f_FileSystem  = NULL;

    EFI_GUID f_LoadedImageGuid   = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID f_FileSystemGuid    = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    f_Status = fp_SystemTable->BootServices->HandleProtocol(fp_ImageHandle, &f_LoadedImageGuid, (VOID**)&f_LoadedImage);

    if (EFI_ERROR(f_Status))
    {
        return f_Status;
    }

    f_Status = fp_SystemTable->BootServices->HandleProtocol(f_LoadedImage->DeviceHandle, &f_FileSystemGuid, (VOID**)&f_FileSystem);

    if (EFI_ERROR(f_Status))
    {
        return f_Status;
    }

    /*
        ----------------------------------------------------------------
        open the root directory of the ESP
        ----------------------------------------------------------------
    */

    EFI_FILE_PROTOCOL* f_RootDir = NULL;

    f_Status = f_FileSystem->OpenVolume(f_FileSystem, &f_RootDir);

    if (EFI_ERROR(f_Status))
    {
        return f_Status;
    }

    /*
        ----------------------------------------------------------------
        open violet_kernel.elf
        ----------------------------------------------------------------
        open in readonly mode uwu;
        the filename is UCS-2/UTF-16 because UEFI string literals are wide
        L"violet_kernel.elf" is the correct way to write a UEFI wide string literal
    */

    EFI_FILE_PROTOCOL* f_KernelFile = NULL;

    f_Status = f_RootDir->Open(f_RootDir, &f_KernelFile, L"violet_kernel.elf", EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(f_Status))
    {
        f_RootDir->Close(f_RootDir);
        return f_Status;
    }

    /*
        ----------------------------------------------------------------
        read the ELF header
        ----------------------------------------------------------------
        read exactly sizeof(Elf64Header) bytes from the start of the file;
        UEFI Read() takes a size by pointer, and it writes back how many bytes
        were actually read, which should match what we asked for
    */

    Elf64Header f_ElfHeader;
    UINTN       f_ReadSize = sizeof(Elf64Header);

    f_Status = f_KernelFile->Read(f_KernelFile, &f_ReadSize, &f_ElfHeader);

    if (EFI_ERROR(f_Status) or f_ReadSize != sizeof(Elf64Header))
    {
        f_KernelFile->Close(f_KernelFile);
        f_RootDir->Close(f_RootDir);
        return EFI_LOAD_ERROR;
    }

    if (not Internal_ValidateElfHeader(&f_ElfHeader))
    {
        f_KernelFile->Close(f_KernelFile);
        f_RootDir->Close(f_RootDir);
        return EFI_LOAD_ERROR;
    }

    /*
        ----------------------------------------------------------------
        read all program headers into memory
        ----------------------------------------------------------------
        program headers start at f_ElfHeader.PhOffset in the file;
        there are f_ElfHeader.PhCount of them, each f_ElfHeader.PhEntrySize bytes;
        seek to that offset by setting the file position, then read the whole table
    */

    f_Status = f_KernelFile->SetPosition(f_KernelFile, f_ElfHeader.ProgramHeaderOffset);

    if (EFI_ERROR(f_Status))
    {
        f_KernelFile->Close(f_KernelFile);
        f_RootDir->Close(f_RootDir);
        return f_Status;
    }

    UINTN f_PhTableSize = (UINTN)f_ElfHeader.ProgramHeaderCount * f_ElfHeader.ProgramHeaderEntrySize;
    Elf64ProgramHeader* f_ProgramHeaders = NULL;

    f_Status = fp_SystemTable->BootServices->AllocatePool(EfiLoaderData, f_PhTableSize, (VOID**)&f_ProgramHeaders);

    if (EFI_ERROR(f_Status))
    {
        f_KernelFile->Close(f_KernelFile);
        f_RootDir->Close(f_RootDir);
        return f_Status;
    }

    f_ReadSize = f_PhTableSize;
    f_Status   = f_KernelFile->Read(f_KernelFile, &f_ReadSize, f_ProgramHeaders);

    if (EFI_ERROR(f_Status) or f_ReadSize != f_PhTableSize)
    {
        fp_SystemTable->BootServices->FreePool(f_ProgramHeaders);
        f_KernelFile->Close(f_KernelFile);
        f_RootDir->Close(f_RootDir);

        return EFI_LOAD_ERROR;
    }

    /*
        ----------------------------------------------------------------
        walk PT_LOAD segments, allocate pages, copy data
        ----------------------------------------------------------------

        idek what the fuck this comment was before but that's not what we were doing LMFAO
    */

    uint64_t f_LoadBase = UINT64_MAX;
    uint64_t f_LoadEnd  = 0;

    for (uint16_t lv_Index = 0; lv_Index < f_ElfHeader.ProgramHeaderCount; lv_Index++)
    {
        /*
            walk using PhEntrySize not sizeof(Elf64ProgramHeader) because it's the same deal as EFI_MEMORY_DESCRIPTOR; 
            the spec allows extensions because idk smth ab mb manufacturers and american megatrends and the other less relevant ones
        */

        Elf64ProgramHeader* f_ProgramHeader = (Elf64ProgramHeader*)((uint8_t*)f_ProgramHeaders + lv_Index*f_ElfHeader.ProgramHeaderEntrySize);

        if (f_ProgramHeader->Type != ELF_PT_LOAD or f_ProgramHeader->MemorySize == 0)
        {
            continue; /* skip non-loadable segments (PT_NOTE, PT_GNU_STACK etc) */
        }

        /*
            compute how many 4KB pages we need for this segment and round MemorySize up to the next page boundary
        */

        UINTN f_PageCount = (f_ProgramHeader->MemorySize + 0xFFF) / 0x1000;

        /*
            load at p_paddr (physical address);
            the linker script sets p_paddr = KERNEL_LMA + section_offset so each segment lands at its correct position in physical RAM starting at 2MB;
            p_vaddr is the runtime virtual address; only valid after page tables are up
        */
        EFI_PHYSICAL_ADDRESS f_PhysicalAddress = (EFI_PHYSICAL_ADDRESS)f_ProgramHeader->PhysicalAddr;

        f_Status = fp_SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, f_PageCount, &f_PhysicalAddress);

        if (EFI_ERROR(f_Status))
        {
            fp_SystemTable->BootServices->FreePool(f_ProgramHeaders);
            f_KernelFile->Close(f_KernelFile);
            f_RootDir->Close(f_RootDir);
            
            return f_Status;
        }

        /*
            zero the entire allocated region first; 
            handles BSS padding aka the gap between FileSize and MemorySize at the end of the segment
        */

        Internal_Memset((void*)f_PhysicalAddress, 0, f_ProgramHeader->MemorySize);

        /*
            seek to this segment's data in the file and read FileSize bytes
            BSS has FileSize == 0 so this correctly copies nothing for pure BSS segments
        */

        if (f_ProgramHeader->FileSize > 0)
        {
            f_Status = f_KernelFile->SetPosition(f_KernelFile, f_ProgramHeader->FileOffset);

            if (EFI_ERROR(f_Status))
            {
                fp_SystemTable->BootServices->FreePool(f_ProgramHeaders);
                f_KernelFile->Close(f_KernelFile);
                f_RootDir->Close(f_RootDir);

                return f_Status;
            }

            f_ReadSize = (UINTN)f_ProgramHeader->FileSize;
            f_Status   = f_KernelFile->Read(f_KernelFile, &f_ReadSize, (void*)f_PhysicalAddress);

            if (EFI_ERROR(f_Status))
            {
                fp_SystemTable->BootServices->FreePool(f_ProgramHeaders);
                f_KernelFile->Close(f_KernelFile);
                f_RootDir->Close(f_RootDir);

                return f_Status;
            }
        }

        /* track physical range the kernel occupies for BootInfo */
        if (f_PhysicalAddress < f_LoadBase)
        {
            f_LoadBase = f_PhysicalAddress;
        }

        uint64_t f_SegmentEnd = f_PhysicalAddress + f_ProgramHeader->MemorySize;

        if (f_SegmentEnd > f_LoadEnd)
        {
            f_LoadEnd = f_SegmentEnd;
        }
    }

    /*
        ----------------------------------------------------------------
        populate output struct and clean up
        ----------------------------------------------------------------
    */

    fp_OutKernel->EntryPoint = f_ElfHeader.EntryPoint;
    fp_OutKernel->LoadBase   = f_LoadBase;
    fp_OutKernel->LoadEnd    = f_LoadEnd;

    fp_SystemTable->BootServices->FreePool(f_ProgramHeaders);
    f_KernelFile->Close(f_KernelFile);
    f_RootDir->Close(f_RootDir);

    return EFI_SUCCESS;
}