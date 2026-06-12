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
#include <Protocol/LoadedImage.h>

///VioletShared
#include "shared/gop/console/GopConsole.h"
#include "shared/BootInfo.h"
#include "shared/utils/StringConversion.h"

#include "shared/arch/DisableInterrupts.h"
#include "shared/arch/Sleep.h"
#include "shared/arch/PageTableOperations.h"
#include "shared/arch/MemoryWidthTypes.h"

///VioletBootloader uwu
#include "MemoryMap.h"
#include "ElfLoader.h"
#include "PageTable.h"
#include "GopInit.h"

#define VIOLET_SPIN_FOREVER() for(;;) {;}

/*
    UEFI firmware finds BOOTX64.EFI on the ESP, parses the PE32+ header, and calls this function with:
        fp_ImageHandle — handle to this EFI application image
        fp_SystemTable — root of everything UEFI gives you: boot services, runtime services, console, protocol handles
            
    EFIAPI enforces the Microsoft x64 calling convention (shadow space etc.)
    clang handles this automatically when targeting x86_64-pc-win32-coff
    but the annotation is explicit documentation of the ABI requirement
*/

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

EFI_STATUS EFIAPI 
    UefiMain
    (
        EFI_HANDLE        fp_ImageHandle,
        EFI_SYSTEM_TABLE* fp_SystemTable
    )
{
    //////////////////////////////////////// Grab the rsp immediately so we get the most accurate version ////////////////////////////////////////

    uint64_t f_InitialRegisterStackPointerValue;
    __asm__ volatile("mov %0, rsp" : "=r"(f_InitialRegisterStackPointerValue));

    //////////////////////////////////////// find the address of the efi so we can map the code as well after we setup the pml4 ////////////////////////////////////////

    EFI_LOADED_IMAGE_PROTOCOL* f_UefiMainImage = NULL;
    EFI_GUID f_UefiMainGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    fp_SystemTable->BootServices->HandleProtocol
    (
        fp_ImageHandle, 
        &f_UefiMainGuid, 
        (VOID**)&f_UefiMainImage
    );

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

    // Add a generous padding of 2 pages (8192 bytes) for any UEFI background allocations
    UINTN f_SafeMapSize = f_DummyMapSize + 2*VIOLET_PAGE_SIZE;
    UINTN f_RequiredMapPages = (f_SafeMapSize + (VIOLET_PAGE_SIZE - 1)) / VIOLET_PAGE_SIZE; // Round up to pages

    //////////////////////////////////////// pre allocate 4 massive, page-aligned pages (16 KiB) right away

    EFI_PHYSICAL_ADDRESS f_MemoryMapDescriptorsPhysicalAddressBase = 0;
    fp_SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, f_RequiredMapPages, &f_MemoryMapDescriptorsPhysicalAddressBase);
    f_MemoryMap.Descriptors = (void*)f_MemoryMapDescriptorsPhysicalAddressBase;

    //////////////////////////////////////// Tell UEFI we have 16 KiB of space, and just grab the map in one shot!

    f_MemoryMap.MapSize = 4 * VIOLET_PAGE_SIZE;

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
        VIOLET_SPIN_FOREVER(); //spin forever to make sure user sees and closes :'(
    }

    VioletGopConsole_PrintLine(&f_VioletConsole, "[SUCCESS]: kernel loaded!");

    //////////////////////////////////////// Setup Basic PageTable sufficient for the kernel to start uwu ////////////////////////////////////////

    uint64_t f_KernelPageCount = (f_Kernel.LoadEnd - f_Kernel.LoadBase + (VIOLET_PAGE_SIZE - 1)) / VIOLET_PAGE_SIZE;

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
        VIOLET_SPIN_FOREVER(); //spin forever to make sure user sees and closes :'(
    }

    //////////////////////////////////////// Map pages for the PMM's bitmap ////////////////////////////////////////

    size_t f_TotalPageCount = (uint64_t)(VioletMemoryMap_GetHighestPhysicalAddress(&f_MemoryMap) / VIOLET_PAGE_SIZE);
    size_t f_BitmapSizeBytes = (f_TotalPageCount + 7) / 8;

    UINTN f_BitmapRequiredPagesAmount = (f_BitmapSizeBytes + (VIOLET_PAGE_SIZE - 1)) / VIOLET_PAGE_SIZE;

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
        VIOLET_SPIN_FOREVER(); // sit on the error, prompt user to reboot ig idk
    }

    // identity map it so the kernel can write to it
    for (uint64_t lv_Page = 0; lv_Page < f_BitmapRequiredPagesAmount; lv_Page++)
    {
        uint64_t fv_PhysicalAddress = f_BitmapPhysicalAddress + lv_Page*VIOLET_PAGE_SIZE;

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

    // map the memory map descriptors across 4 pages, 16KiB should be more than enough to handle them uwu

    for (uint64_t lv_Page = 0; lv_Page < 4; lv_Page++)
    {
        uint64_t fv_PhysicalAddress = f_MemoryMapDescriptorsPhysicalAddressBase + lv_Page*VIOLET_PAGE_SIZE;

        Violet_MapPage
        (
            fp_SystemTable, 
            f_Pml4, 
            fv_PhysicalAddress, fv_PhysicalAddress, 
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE | PAGE_TABLE_ENTRY_NO_EXECUTE
        );
    }

    //////////////////////////////////////// Grab the location of UefiMain ////////////////////////////////////////

    uint64_t f_BootloaderCodeBase = (uint64_t)f_UefiMainImage->ImageBase;
    uint64_t f_BootloaderCodePageAmount = (f_UefiMainImage->ImageSize + (VIOLET_PAGE_SIZE - 1)) / VIOLET_PAGE_SIZE;

    for (uint64_t lv_Page = 0; lv_Page < f_BootloaderCodePageAmount; lv_Page++)
    {
        uint64_t f_Address = f_BootloaderCodeBase + lv_Page*VIOLET_PAGE_SIZE;

        Violet_MapPage
        (
            fp_SystemTable, 
            f_Pml4, 
            f_Address, f_Address, 
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE
        );
    }

    f_BootInfo->BootloaderCodeBase       = f_BootloaderCodeBase;
    f_BootInfo->BootloaderCodePageAmount = f_BootloaderCodePageAmount;

    //////////////////////////////////////// identity map the uefimain stack so we don't unmap our stack frame when we do our cr3 jump ////////////////////////////////////////

    uint64_t f_StackBlockTarget          = f_InitialRegisterStackPointerValue;
    uint64_t f_BootLoaderStackBase       = 0;
    uint64_t f_BootLoaderStackPageAmount = 0;

    /*
        essentially we loop through each UEFI memory descriptor because the memory region associated with UefiMain's stack frame will be there;
        so we just look for the region of memory that contains the register stack pointer address we cached at the beginning of the function owo
        this ensures that we capture the entire region to avoid weird edge cases, as well as fully utilize the memory given to us for the bootloader >w<
        it should be 128 KiB at the very least for any motherboard(probably the uefi vendor let's be real) that is UEFI compliant
    */

    for (size_t lv_Index = 0; lv_Index < f_MemoryMap.DescriptorCount; lv_Index++)
    {
        EFI_MEMORY_DESCRIPTOR* fv_Descriptor = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)f_MemoryMap.Descriptors + (lv_Index * f_MemoryMap.DescriptorSize));

        uint64_t lv_Start = fv_Descriptor->PhysicalStart;
        uint64_t lv_End   = lv_Start + (fv_Descriptor->NumberOfPages * VIOLET_PAGE_SIZE);

        if (f_StackBlockTarget >= lv_Start and f_StackBlockTarget < lv_End) // Does our RSP fall inside this allocated UEFI memory region?
        {
            f_BootLoaderStackBase = lv_Start;
            f_BootLoaderStackPageAmount = fv_Descriptor->NumberOfPages;

            break;
        }
    }

    if(f_BootLoaderStackBase == 0 or f_BootLoaderStackPageAmount == 0) //error out otherwise, but this should never be the case; at least somebody gets the full info tho ;w;
    {
        VioletGopConsole_PrintLine(&f_VioletConsole, "[BOOT FAILED]: unable to identify memory region associated with UefiMain()'s stack frame ;w;");
        VIOLET_SPIN_FOREVER();
    }

    VioletGopConsole_Print(&f_VioletConsole, "[INFO]: found memory region associated with uefi main's stack, number of pages: ");
    VioletGopConsole_PrintLine(&f_VioletConsole, uintn_to_str(f_BootLoaderStackPageAmount));

    VioletArch_SleepCycles(10'000'000'000);

    /* 
        Now map EXACTLY the boundaries UEFI guaranteed us, not a single page more or less;
        the spec guarantees at least 32 pages or 16 KiB but sometimes we can be given more, some mb's are more generous lovers >///<
    */

    for (uint64_t lv_Page = 0; lv_Page < f_BootLoaderStackPageAmount; lv_Page++)
    {
        uint64_t f_PhysicalAddress = f_BootLoaderStackBase + (lv_Page * VIOLET_PAGE_SIZE); // physical start from uefi maps the lower floor address, so we can grow up the stack uwu

        Violet_MapPage
        (
            fp_SystemTable, 
            f_Pml4, 
            f_PhysicalAddress, f_PhysicalAddress, //identity map the uefi main stack
            PAGE_TABLE_ENTRY_PRESENT | PAGE_TABLE_ENTRY_WRITABLE
        );
    }

    f_BootInfo->BootloaderStackBase       = f_BootLoaderStackBase;
    f_BootInfo->BootloaderStackPageAmount = f_BootLoaderStackPageAmount;

    //////////////////////////////////////// identity map the framebuffer since it's MMIO ////////////////////////////////////////

    uint64_t f_FrameBufferSize  = (uint64_t)f_ConsoleFrameBuffer.Height * f_ConsoleFrameBuffer.PixelsPerScanLine * 4; // size =  height * pixelsPerScanLine * 4 bytes, round up to pages
    uint64_t f_RequiredFrameBufferPages = (f_FrameBufferSize + (VIOLET_PAGE_SIZE - 1)) / VIOLET_PAGE_SIZE;

    for (uint64_t lv_Page = 0; lv_Page < f_RequiredFrameBufferPages; lv_Page++)
    {
        uint64_t f_Address = f_ConsoleFrameBuffer.FrameBufferBase + lv_Page*VIOLET_PAGE_SIZE;

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
        f_MemoryMap.MapSize = f_RequiredMapPages * VIOLET_PAGE_SIZE; // Reset size to our 16 KiB max
        
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
            VioletArch_SleepCycles(1'000'000'000);
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

    VioletArch_DisableInterrupts();

    //////////////////////////////////////// Load the basic page tables using the inline asm for cr3 uwu ////////////////////////////////////////

    VioletArch_LoadPageTable(f_Pml4);

    ////////////////////////////////////////
    // THE STACK EXPLODER 9000
    ////////////////////////////////////////

    // We grab the stack pointer.
    uint64_t f_CurrentRsp;
    __asm__ volatile("mov %0, rsp" : "=r"(f_CurrentRsp));

    // We are going to force the CPU to write a byte 5 pages (20KB) BELOW our current stack.
    // Because we only mapped UP, this memory is totally unmapped in our new cr3
    volatile char* f_ExplodePtr = (volatile char*)(f_CurrentRsp - (5 * VIOLET_PAGE_SIZE));

    // The moment this line executes, the CPU MMU will look at our new page tables, see that this page isn't present, throw a #PF (Page Fault), and my machine will hang/reboot.
    *f_ExplodePtr = 'F';

    ////////////////////////////////////////// Get Kernel Main PFN

    // we need the System V ABI since we're booting from an elf
    typedef void __attribute__((sysv_abi)) (*KernelEntryFunc)(VioletBoot_Info*);

    KernelEntryFunc f_KernelEntry = (KernelEntryFunc)f_Kernel.EntryPoint;

    // //////////////////////////////////////// Call Kernel Main!

    f_KernelEntry(f_BootInfo);

    return EFI_SUCCESS; // sorry you're never getting called ;w;
}