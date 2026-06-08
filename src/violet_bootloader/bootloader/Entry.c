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

///UEFI
#include <Uefi.h>

///VioletShared
#include "shared/gop/console/GopConsole.h"
#include "shared/BootInfo.h"

///VioletBootloader uwu
#include "MemoryMap.h"
#include "ElfLoader.h"
#include "PageTable.h"
#include "GopInit.h"

const char* 
    uintn_to_str(size_t value) 
{
    // A 64-bit integer requires at most 20 digits + 1 null terminator
    static char buffer[21]; 
    char* ptr = &buffer[20];
    *ptr = '\0'; // Null terminate the string

    // Special case for 0
    if (value == 0) 
    {
        *--ptr = '0';
        return ptr;
    }

    // Process digits from right to left
    while (value > 0) 
    {
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
    VioletBoot_Info f_BootInfo;

    /*
        CHECK!: init GOP framebuffer (Gop.h)
        CHECK!: print something to screen so we know we're alive
        CHECK!: get UEFI memory map (MemoryMap.h)
        CHECK!: load kernel ELF from ESP (ElfLoader.h)
        CHECK!: set up initial page tables
        CHECK!: call ExitBootServices() and can UEFI boot services forever (or at least until the next startup >w<)
        CHECK!: jump to KernelMain

        we have a minimal bootloader working >///<
    */

    //////////////////////////////////////// Initialize FrameBuffer ////////////////////////////////////////

    EFI_STATUS f_Status = Violet_InitGop(fp_SystemTable, &f_BootInfo.FrameBuffer);

    //////////////////////////////////////// Print something to make sure we're alive uwu ////////////////////////////////////////

    if (EFI_ERROR(f_Status))
    {
        return f_Status;
    }

    VioletGopFrameBuffer_ClearScreen(&f_BootInfo.FrameBuffer, VIOLET_COLOUR_VIOLET);  // 💜

    VioletConsole f_VioletConsole = VioletConsole_Create(&f_BootInfo.FrameBuffer, VIOLET_COLOUR_WHITE, VIOLET_COLOUR_VIOLET);
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

    //////////////////////////////////////// Setup Basic PageTable sufficient for the kernel to start uwu ////////////////////////////////////////

    uint64_t f_KernelPageCount = (f_Kernel.LoadEnd - f_Kernel.LoadBase + 0xFFF) / 0x1000;

    uint64_t f_Pml4 = Violet_SetupPageTables
    (
        fp_SystemTable,
        f_Kernel.LoadBase,       // 0x200000
        0xFFFFFFFF80000000ULL,   // kernel VMA from linker script
        f_KernelPageCount
    );

    if (f_Pml4 == 0)
    {
        VioletConsole_PrintLine(&f_VioletConsole, "page table setup failed ;w;");
        for (;;) { ; } //spin forever to make sure user sees and closes :'(
    }

    //////////////////////////////////////// Grab the location of UefiMain ////////////////////////////////////////

    uint64_t f_BootloaderCodeBase = (uint64_t)UefiMain & ~0xFFFULL;
    for (uint64_t lv_Page = 0; lv_Page < 1024; lv_Page++)
    {
        uint64_t f_Address = f_BootloaderCodeBase + lv_Page * 0x1000;

        Violet_MapPage
        (
            fp_SystemTable, 
            f_Pml4, 
            f_Address, f_Address, 
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE
        );
    }

    //////////////////////////////////////// identity map the bootloader stack and grab the current RSP register value so we don't unmap our stack frame ////////////////////////////////////////
    
    uint64_t f_RegisterStackPointerValue;
    __asm__ volatile("mov %%rsp, %0" : "=r"(f_RegisterStackPointerValue));
    
    /* Shift base down 64 KiB (16 pages) to capture the stack tail, then map 128 KiB (32 pages) total to cover above and below RSP */
    
    uint64_t f_BootLoaderStackBase = (f_RegisterStackPointerValue & ~0xFFFULL) - (16 * 0x1000);
    for (uint64_t lv_Page = 0; lv_Page < 32; lv_Page++)
    {
        uint64_t f_Address = f_BootLoaderStackBase + lv_Page*0x1000;

        Violet_MapPage
        (
            fp_SystemTable, 
            f_Pml4, 
            f_Address, f_Address, 
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE
        );
    }

    //////////////////////////////////////// identity map the framebuffer since the it's MMIO ////////////////////////////////////////

    uint64_t f_FrameBufferSize  = (uint64_t)f_BootInfo.FrameBuffer.Height * f_BootInfo.FrameBuffer.PixelsPerScanLine * 4; // size =  height * pixelsPerScanLine * 4 bytes, round up to pages
    uint64_t f_RequiredFrameBufferPages = (f_FrameBufferSize + 0xFFF) / 0x1000;

    for (uint64_t lv_Page = 0; lv_Page < f_RequiredFrameBufferPages; lv_Page++)
    {
        uint64_t f_Address = f_BootInfo.FrameBuffer.FrameBufferBase + lv_Page * 0x1000;

        Violet_MapPage
        (
            fp_SystemTable, 
            f_Pml4, 
            f_Address, f_Address, // same since its identity mapped uwu
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE | PAGE_TABLE_ENTRY_NO_EXECUTE
        );
    }

    //////////////////////////////////////// NOW we can exit boot services owo fingys cwossed >w< ////////////////////////////////////////

    EFI_STATUS f_ExitStatus;
        
    while (1) 
    {
        // Get the current size of the memory map
        f_MemoryMap.MapSize = 0;
        fp_SystemTable->BootServices->GetMemoryMap(&f_MemoryMap.MapSize, NULL, &f_MemoryMap.MapKey, &f_MemoryMap.DescriptorSize, &f_MemoryMap.DescriptorVersion);
        
        // Pad the size! Getting the map AND exiting boot services can trigger allocations;
        // Adding 4 extra descriptors worth of padding is the standard UEFI safe margin
        f_MemoryMap.MapSize += 4 * f_MemoryMap.DescriptorSize;
        
        // Allocate memory for the map (We must free the old one if we are looping, but usually this works on pass 1 or 2)
        if (f_MemoryMap.Descriptors != NULL) 
        {
            fp_SystemTable->BootServices->FreePool(f_MemoryMap.Descriptors);
        }

        fp_SystemTable->BootServices->AllocatePool(EfiLoaderData, f_MemoryMap.MapSize, (VOID**)&f_MemoryMap.Descriptors);
        
        // Actually get the memory map and the definitive MapKey
        fp_SystemTable->BootServices->GetMemoryMap(&f_MemoryMap.MapSize, f_MemoryMap.Descriptors, &f_MemoryMap.MapKey, &f_MemoryMap.DescriptorSize, &f_MemoryMap.DescriptorVersion);
        
        // Try to exit >w<
        f_ExitStatus = fp_SystemTable->BootServices->ExitBootServices(fp_ImageHandle, f_MemoryMap.MapKey);
        
        // If we succeed, break the loop; otherwise we try again ;w;
        if (f_ExitStatus == EFI_SUCCESS) 
        {
            VioletConsole_PrintLine(&f_VioletConsole, "[SUCCESS]: exited boot services safely >///<");
            break;
        }
        else
        {
            VioletConsole_PrintLine(&f_VioletConsole, "[FAILED]: Unable to exit boot services, trying again...");
        }
    }
    
    //////////////////////////////////////// Now we hand off the memory map values over to the C std values uwu ////////////////////////////////////////

    f_BootInfo.MemoryMap.Descriptors = (uint8_t*)f_MemoryMap.Descriptors;

    f_BootInfo.MemoryMap.MapSize           = f_MemoryMap.MapSize;
    f_BootInfo.MemoryMap.DescriptorCount   = f_MemoryMap.DescriptorCount;
    f_BootInfo.MemoryMap.MapKey            = f_MemoryMap.MapKey;
    f_BootInfo.MemoryMap.DescriptorSize    = f_MemoryMap.DescriptorSize;
    f_BootInfo.MemoryMap.DescriptorVersion = f_MemoryMap.DescriptorVersion;

    //////////////////////////////////////// Load the basic page tables using the inline asm for cr3 uwu

    // CRITICAL: Shut off interrupts! UEFI's handlers are about to be unmapped.
    __asm__ volatile("cli");

    Violet_LoadCr3(f_Pml4);

    ////////////////////////////////////////// Get Kernel Main PFN

    // we need the System V ABI since we're booting from an elf
    typedef void __attribute__((sysv_abi)) (*KernelEntryFunc)(VioletBoot_Info*);

    KernelEntryFunc f_KernelEntry = (KernelEntryFunc)f_Kernel.EntryPoint;

    // //////////////////////////////////////// Call Kernel Main!

    f_KernelEntry(&f_BootInfo);

    return EFI_SUCCESS;
}