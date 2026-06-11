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
#include "shared/utils/StringConversion.h"

#include "shared/arch/DisableInterrupts.h"
#include "shared/arch/Sleep.h"

///VioletBootloader uwu
#include "MemoryMap.h"
#include "ElfLoader.h"
#include "PageTable.h"
#include "GopInit.h"

#define VIOLET_X86_PAGE_SIZE 0x1000

#define VIOLET_SPIN_FOREVER for(;;) {;}

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
    /*
        CHECK!: init GOP framebuffer (Gop.h)
        CHECK!: print something to screen so we know we're alive
        CHECK!: get UEFI memory map (MemoryMap.h)
        CHECK!: load kernel ELF from ESP (ElfLoader.h)
        CHECK!: set up initial page tables
        CHECK!: map pages required for the PMM bitmap
        CHECK!: call ExitBootServices() and can UEFI boot services forever (or at least until the next startup >w<)
        CHECK!: jump to KernelMain

        we have a minimal bootloader working >///<
    */

    //////////////////////////////////////// Initialize FrameBuffer ////////////////////////////////////////

    VioletGop_FrameBuffer f_ConsoleFrameBuffer;

    EFI_STATUS f_Status = Violet_InitGop(fp_SystemTable, &f_ConsoleFrameBuffer);

    if (EFI_ERROR(f_Status))
    {
        return f_Status;
    }

    //////////////////////////////////////// Print something to make sure we're alive uwu ////////////////////////////////////////

    VioletGopFrameBuffer_ClearScreen(&f_ConsoleFrameBuffer, VIOLET_COLOUR_VIOLET);  // 💜

    VioletGop_Console f_VioletConsole = VioletGopConsole_Create(&f_ConsoleFrameBuffer, VIOLET_COLOUR_WHITE, VIOLET_COLOUR_VIOLET);
    VioletGopConsole_PrintLine(&f_VioletConsole, "Hello World!");

    //////////////////////////////////////// Get the UEFI memory map ////////////////////////////////////////

    VioletMemoryMap f_MemoryMap = VioletMemoryMap_CreateEmpty();

    //////////////////////////////////////// Get the size of the UEFI memory map

    UINTN f_DummyMapSize = 0;
    UINTN f_MapKey, f_DescSize;
    UINT32 f_DescVersion;

    fp_SystemTable->BootServices->GetMemoryMap(&f_DummyMapSize, NULL, &f_MapKey, &f_DescSize, &f_DescVersion);

    // 2. Add a generous padding of 2 pages (8192 bytes) for any UEFI background allocations
    UINTN f_SafeMapSize = f_DummyMapSize + 0x2000;
    UINTN f_RequiredMapPages = (f_SafeMapSize + 0xFFF) / 0x1000; // Round up to pages

    //////////////////////////////////////// pre allocate 4 massive, page-aligned pages (16 KiB) right away

    EFI_PHYSICAL_ADDRESS f_MemoryMapPhysicalAddress = 0;
    fp_SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, f_RequiredMapPages, &f_MemoryMapPhysicalAddress);
    f_MemoryMap.Descriptors = (void*)f_MemoryMapPhysicalAddress;

    //////////////////////////////////////// Tell UEFI we have 16 KiB of space, and just grab the map in one shot!

    f_MemoryMap.MapSize = 4 * 0x1000;

    fp_SystemTable->BootServices->GetMemoryMap
    (
        &f_MemoryMap.MapSize, 
        f_MemoryMap.Descriptors, 
        &f_MemoryMap.MapKey, 
        &f_MemoryMap.DescriptorSize, 
        &f_MemoryMap.DescriptorVersion
    );

    f_MemoryMap.DescriptorCount = f_MemoryMap.MapSize / f_MemoryMap.DescriptorSize;

    //////////////////////////////////////// Load Kernel Elf ////////////////////////////////////////

    VioletLoadedKernel f_Kernel;
    f_Status = Violet_LoadKernelElf(fp_ImageHandle, fp_SystemTable, &f_Kernel, &f_VioletConsole);

    if (EFI_ERROR(f_Status))
    {
        VioletGopConsole_PrintLine(&f_VioletConsole, "failed to load kernel elf ;w;");
        VIOLET_SPIN_FOREVER //spin forever to make sure user sees and closes :'(
    }

    VioletGopConsole_PrintLine(&f_VioletConsole, "[SUCCESS]: kernel loaded!");

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
        VioletGopConsole_PrintLine(&f_VioletConsole, "page table setup failed ;w;");
        for (;;) { ; } //spin forever to make sure user sees and closes :'(
    }

    //////////////////////////////////////// Map pages for the PMM's bitmap ////////////////////////////////////////

    size_t f_TotalPageCount = (uint64_t)(VioletMemoryMap_GetHighestPhysicalAddress(&f_MemoryMap) / 0x1000);
    size_t f_BitmapSizeBytes = (f_TotalPageCount + 7) / 8;

    UINTN f_BitmapRequiredPagesAmount = (f_BitmapSizeBytes + 0xFFF) / 0x1000;

    VioletGopConsole_Print(&f_VioletConsole, "Required Pages: ");
    VioletGopConsole_PrintLine(&f_VioletConsole, uintn_to_str(f_BitmapRequiredPagesAmount)); // ain't doing str ops yet lmfao

    // find a suitable region for the bitmap
    EFI_PHYSICAL_ADDRESS f_BitmapPhysicalAddress = 0;

    // get UEFI to find some pages for us, ily tianocore
    f_Status = fp_SystemTable->BootServices->AllocatePages
    (
        AllocateAnyPages,
        EfiLoaderData,
        f_BitmapRequiredPagesAmount,
        &f_BitmapPhysicalAddress
    );

    if(EFI_ERROR(f_Status))
    {
        VioletGopConsole_PrintLine(&f_VioletConsole, "[BOOT FAILED]: no viable region found for PMM bitmap");
        for(;;) { ; } // sit on the error, prompt user to reboot ig idk
    }

    // identity map it so the kernel can write to it
    for (uint64_t lv_Page = 0; lv_Page < f_BitmapRequiredPagesAmount; lv_Page++)
    {
        uint64_t fv_PhysicalAddress = f_BitmapPhysicalAddress + lv_Page*0x1000;

        Violet_MapPage
        (
            fp_SystemTable, 
            f_Pml4, 
            fv_PhysicalAddress, fv_PhysicalAddress,
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE | PAGE_TABLE_ENTRY_NO_EXECUTE
        );
    }

    //////////////////////////////////////// Allocate and Map Boot Info uwu ////////////////////////////////////////

    VioletBoot_Info* f_BootInfo;

    fp_SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, (EFI_PHYSICAL_ADDRESS*)&f_BootInfo);

    Violet_MapPage
    (
        fp_SystemTable, 
        f_Pml4, 
        (uint64_t)f_BootInfo, (uint64_t)f_BootInfo, 
        PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE | PAGE_TABLE_ENTRY_NO_EXECUTE
    );

    f_BootInfo->BitmapPhysicalAddress  = (uint64_t)f_BitmapPhysicalAddress;
    f_BootInfo->BitmapSizeBytes        = f_BitmapSizeBytes;
    f_BootInfo->TotalPageCount         = f_TotalPageCount;
    f_BootInfo->FrameBuffer            = f_ConsoleFrameBuffer;

    //////////////////////////////////////// Map the Memory Map Buffer ////////////////////////////////////////

    // Map all 4 pages of our memory map buffer into the kernel page tables
    for (uint64_t lv_Page = 0; lv_Page < 4; lv_Page++)
    {
        uint64_t fv_PhysicalAddress = f_MemoryMapPhysicalAddress + (lv_Page * 0x1000);

        Violet_MapPage
        (
            fp_SystemTable, 
            f_Pml4, 
            fv_PhysicalAddress, fv_PhysicalAddress, 
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE | PAGE_TABLE_ENTRY_NO_EXECUTE
        );
    }

    //////////////////////////////////////// Grab the location of UefiMain ////////////////////////////////////////

    uint64_t f_BootloaderCodeBase = (uint64_t)UefiMain & ~0xFFFULL;
    for (uint64_t lv_Page = 0; lv_Page < 1024; lv_Page++)
    {
        uint64_t f_Address = f_BootloaderCodeBase + lv_Page*0x1000;

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

    //////////////////////////////////////// identity map the framebuffer since it's MMIO ////////////////////////////////////////

    uint64_t f_FrameBufferSize  = (uint64_t)f_ConsoleFrameBuffer.Height * f_ConsoleFrameBuffer.PixelsPerScanLine * 4; // size =  height * pixelsPerScanLine * 4 bytes, round up to pages
    uint64_t f_RequiredFrameBufferPages = (f_FrameBufferSize + 0xFFF) / 0x1000;

    for (uint64_t lv_Page = 0; lv_Page < f_RequiredFrameBufferPages; lv_Page++)
    {
        uint64_t f_Address = f_ConsoleFrameBuffer.FrameBufferBase + lv_Page*0x1000;

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
        f_MemoryMap.MapSize = f_RequiredMapPages * 0x1000; // Reset size to our 16 KiB max
        
        fp_SystemTable->BootServices->GetMemoryMap
        (
            &f_MemoryMap.MapSize, 
            f_MemoryMap.Descriptors, 
            &f_MemoryMap.MapKey, 
            &f_MemoryMap.DescriptorSize, 
            &f_MemoryMap.DescriptorVersion
        );
        
        f_ExitStatus = fp_SystemTable->BootServices->ExitBootServices(fp_ImageHandle, f_MemoryMap.MapKey);

        // If we succeed, break the loop; otherwise we try again ;w;
        if (f_ExitStatus == EFI_SUCCESS) 
        {
            VioletGopConsole_PrintLine(&f_VioletConsole, "[SUCCESS]: exited boot services safely >///<");
            break;
        }
        else
        {
            VioletGopConsole_PrintLine(&f_VioletConsole, "[FAILED]: Unable to exit boot services, trying again...");
            VIOLET_SLEEP_FOR_CYCLES(10'000'000'000);
        }
    }
    
    //////////////////////////////////////// Now we hand off the memory map values over to the C std values uwu ////////////////////////////////////////

    // MUST recalculate this here because MapSize was changed by UEFI inside the loop ;w;
    f_MemoryMap.DescriptorCount = f_MemoryMap.MapSize / f_MemoryMap.DescriptorSize;

    f_BootInfo->MemoryMap.Descriptors = (uint8_t*)f_MemoryMap.Descriptors;

    f_BootInfo->MemoryMap.MapSize           = f_MemoryMap.MapSize;
    f_BootInfo->MemoryMap.DescriptorCount   = f_MemoryMap.DescriptorCount;
    f_BootInfo->MemoryMap.MapKey            = f_MemoryMap.MapKey;
    f_BootInfo->MemoryMap.DescriptorSize    = f_MemoryMap.DescriptorSize;
    f_BootInfo->MemoryMap.DescriptorVersion = f_MemoryMap.DescriptorVersion;

    //////////////////////////////////////// Shut off interrupts since we're about to unmap UEFI ////////////////////////////////////////

    VIOLET_DISABLE_INTERRUPTS;

    //////////////////////////////////////// Load the basic page tables using the inline asm for cr3 uwu ////////////////////////////////////////

    Violet_LoadCr3(f_Pml4);

    ////////////////////////////////////////// Get Kernel Main PFN

    // we need the System V ABI since we're booting from an elf
    typedef void __attribute__((sysv_abi)) (*KernelEntryFunc)(VioletBoot_Info*);

    KernelEntryFunc f_KernelEntry = (KernelEntryFunc)f_Kernel.EntryPoint;

    // //////////////////////////////////////// Call Kernel Main!

    f_KernelEntry(f_BootInfo);

    return EFI_SUCCESS;
}