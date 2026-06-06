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

#include <Uefi.h>
#include "shared/console/GopConsole.h"

#include "MemoryMap.h"
#include "ElfLoader.h"

const char* uintn_to_str(size_t value) 
{
    // A 64-bit integer requires at most 20 digits + 1 null terminator
    static char buffer[21]; 
    char* ptr = &buffer[20];
    *ptr = '\0'; // Null terminate the string

    // Special case for 0
    if (value == 0) {
        *--ptr = '0';
        return ptr;
    }

    // Process digits from right to left
    while (value > 0) {
        *--ptr = (char)('0' + (value % 10));
        value /= 10;
    }

    return ptr;
}

/*
    UefiMain — UEFI application entry point
    UEFI firmware finds BOOTX64.EFI on the ESP, parses the PE32+ header,
    and calls this function with:
        fp_ImageHandle — handle to this EFI application image
        fp_SystemTable — root of everything UEFI gives you:
            boot services, runtime services, console, protocol handles
            
    EFIAPI enforces the Microsoft x64 calling convention (shadow space etc.)
    clang handles this automatically when targeting x86_64-pc-win32-coff
    but the annotation is explicit documentation of the ABI requirement
*/
EFI_STATUS EFIAPI 
    UefiMain
    (
        EFI_HANDLE        fp_ImageHandle,
        EFI_SYSTEM_TABLE* fp_SystemTable
    )
{
    (void)fp_ImageHandle; /* suppress unused warning until we need it */

    /*
        CHECK!: init GOP framebuffer (Gop.h)
        CHECK!: print something to screen so we know we're alive
        CHECK!: get UEFI memory map (MemoryMap.h)
        CHECK!: load kernel ELF from ESP (ElfLoader.h)
        TODO: set up initial page tables
        TODO: call ExitBootServices() and can UEFI boot services forever (or at least until the next startup >w<)
        TODO: jump to KernelMain
    */

    //////////////////////////////////////// Initialize FrameBuffer ////////////////////////////////////////

    VioletFrameBuffer f_ConsoleFrameBuffer;
    EFI_STATUS f_Status = VioletInitGop(fp_SystemTable, &f_ConsoleFrameBuffer);

    //////////////////////////////////////// Print something to make sure we're alive uwu ////////////////////////////////////////

    if (EFI_ERROR(f_Status))
    {
        return f_Status;
    }

    VioletClearScreen(&f_ConsoleFrameBuffer, VIOLET_COLOUR_VIOLET);  // 💜

    VioletConsole f_VioletConsole = VioletConsole_Create(&f_ConsoleFrameBuffer, VIOLET_COLOUR_WHITE, VIOLET_COLOUR_VIOLET);
    VioletConsole_PrintLine(&f_VioletConsole, "Hello World!");

    //////////////////////////////////////// Get the UEFI memory map ////////////////////////////////////////

    VioletMemoryMap f_MemoryMap = VioletMemoryMap_CreateEmpty();

    //////////////////////////////////////// get the memory map

    fp_SystemTable->BootServices->GetMemoryMap(&f_MemoryMap.MapSize, NULL, &f_MemoryMap.MapKey, &f_MemoryMap.DescriptorSize, &f_MemoryMap.DescriptorVersion);

    //////////////////////////////////////// pad by 2 extra descriptors, the AllocatePool call adds an entry

    f_MemoryMap.MapSize += 2*f_MemoryMap.DescriptorSize;

    fp_SystemTable->BootServices->AllocatePool(EfiLoaderData, f_MemoryMap.MapSize, (VOID**)&f_MemoryMap.Descriptors);

    //////////////////////////////////////// actually get the map

    fp_SystemTable->BootServices->GetMemoryMap(&f_MemoryMap.MapSize, f_MemoryMap.Descriptors, &f_MemoryMap.MapKey, &f_MemoryMap.DescriptorSize, &f_MemoryMap.DescriptorVersion);

    //////////////////////////////////////// Calculate Descriptor Count

    f_MemoryMap.DescriptorCount = f_MemoryMap.MapSize / f_MemoryMap.DescriptorSize;

    //////////////////////////////////////// Get Usable memory pages owo

    UINTN f_UsablePagesCount = VioletMemoryMap_CountUsablePages(&f_MemoryMap);

    VioletConsole_Print(&f_VioletConsole, "Available Memory Pages: ");
    VioletConsole_Print(&f_VioletConsole, uintn_to_str(f_UsablePagesCount));
    VioletConsole_Print(&f_VioletConsole, "\n");

    //////////////////////////////////////// Load Kernel Elf

    VioletLoadedKernel f_Kernel;
    f_Status = Violet_LoadKernelElf(fp_ImageHandle, fp_SystemTable, &f_Kernel, &f_VioletConsole);

    if (EFI_ERROR(f_Status))
    {
        VioletConsole_PrintLine(&f_VioletConsole, "failed to load kernel elf ;w;");
        for (;;) { ; } //spin forever to make sure user sees and closes :'(
    }

    VioletConsole_PrintLine(&f_VioletConsole, "[SUCCESS]: kernel loaded!");

    /* do nothing forever until poweroff ig uwu */

    for (;;)
    {
        ;
    }

    return EFI_SUCCESS;
}