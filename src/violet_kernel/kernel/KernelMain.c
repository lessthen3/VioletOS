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

///CSTD
#include <stdint.h>
#include <stddef.h>

///VioletShared
#include "shared/BootInfo.h"
#include "shared/gop/GopConsole.h"
#include "shared/utils/StringConversion.h"
#include "shared/arch/Sleep.h"
#include "shared/arch/Memory.h"

///VioletKernel
#include "VioletPanic.h"

#include "kernel/memory/PhysicalMemoryManager.h"
#include "kernel/memory/VirtualMemoryManager.h"

#include "kernel/vklib/memset.h"

/*
    these symbols are defined by violet_kernel.ld, apparently better to use an array instead.
    ik it decays to a ptr but like apparently it's more correct than uint8_t* idfk

    future ryan: oh it's because its just a ptr, apparently uint8_t* alone will cause a dereference at some point
    for some reason lmfao idk, writing a compiler requires at least a few assumptions about memory layout i dont blame em
*/

extern uint8_t VioletTextStart[];
extern uint8_t VioletTextEnd[];

extern uint8_t VioletRodataStart[];
extern uint8_t VioletRodataEnd[];

extern uint8_t VioletDataStart[];
extern uint8_t VioletDataEnd[];

extern uint8_t VioletBssStart[];
extern uint8_t VioletBssEnd[];

extern uint8_t VioletKernelEnd[];
extern uint8_t VioletKernelPhysicalEnd[];

/*
    KernelMain: the kernel entry point
    called by the bootloader after:
        - ExitBootServices() has been called
        - a basic page table mapping kernel VMA -> physical load address is set up
        - a valid stack pointer is set up
        - a BootInfo struct is passed describing the memory map, framebuffer, etc

    the __attribute__((section(".text.boot"))) puts this function in the .text.boot
    subsection which violet_kernel.ld places FIRST in .text
    this guarantees KernelMain is at the ELF entry point address
*/

    /*
        CHECK!: accept BootInfo* parameter from bootloader once Entry.c is written
        CHECK!: init PMM from memory map
        CHECK!: init VMM
        TODO: init GDT/IDT
        TODO: init LAPIC
        TODO: spawn root task
    */

__attribute__((section(".text.boot")))
[[noreturn]] void 
    KernelMain(VioletBoot_Info* fp_BootInfo) //can't return since we completely nuked uefi main here owo and boot serivces was exited, only things running now are the uefi runtime services
{
    //////////////////////////////////////// FIRST THING: zero the BSS segment ////////////////////////////////////////
    /*
        crt0 doesnt exist so nobody else will do this, so all uninitialized globals are garbage until we do this
    */
    memset_as_u8(VioletBssStart, 0, (size_t)(VioletBssEnd - VioletBssStart));

    //////////////////////////////////////// Pass GOP framebuffer we got from the bootloader so we can panic safely with some info ////////////////////////////////////////

    VioletPanic_Init(&fp_BootInfo->FrameBuffer);

    //////////////////////////////////////// Initialize the PMM using the memory map from the bootloader ////////////////////////////////////////

    VioletPmm_Init(fp_BootInfo, (phys_addr_t)VioletKernelPhysicalEnd);

    //////////////////////////////////////// Initialize the Vmm now uwu ////////////////////////////////////////

    VioletVmm_Init(fp_BootInfo);

    //////////////////////////////////////// Setup a console for now until we have a DE ////////////////////////////////////////

    VioletGopFrameBuffer_ClearScreen(&fp_BootInfo->FrameBuffer, VIOLET_COLOUR_GREEN);  // owo

    VioletGop_Console f_VioletConsole = VioletGopConsole_Create(&fp_BootInfo->FrameBuffer, VIOLET_COLOUR_WHITE, VIOLET_COLOUR_VIOLET);
    VioletGopConsole_PrintLine(&f_VioletConsole, "Hello World from the Kernel!");

    //////////////////////////////////////////////////////////////////////////////// Random Testing Stuff ////////////////////////////////////////////////////////////////////////////////

    // now deliberately overwrite the freed UEFI stack pages with 0xBABECAFE
    // if we were still using them as a stack this would corrupt return addresses and triple fault
    // if this doesn't crash: stack switch worked, PMM free worked, we're clean
    uint32_t* f_OldStack      = (uint32_t*)fp_BootInfo->BootloaderStackBase;
    size_t    f_OldStackWords = (fp_BootInfo->BootloaderStackPageAmount * 0x1000) / sizeof(uint32_t);

    for (size_t lv_Index = 0; lv_Index < f_OldStackWords; lv_Index++)
    {
        f_OldStack[lv_Index] = 0xBABECAFE;
    }

    VioletGopConsole_PrintLine(&f_VioletConsole, "UEFI stack trashed successfully, still alive 💜");
    VioletGopConsole_PrintLine(&f_VioletConsole, "BSS boot stack confirmed working");

    VioletGopConsole_Print(&f_VioletConsole, "Value from BootInfo after axing stack: ");
    VioletGopConsole_PrintLine(&f_VioletConsole, uintn_to_str(fp_BootInfo->BitmapPhysicalAddress));

    //////////////////////////////////////// VMM Stress Test ////////////////////////////////////////

    VioletVmm_AddressSpace* f_KernelSpace = VioletVmm_GetKernelSpace();

    // allocate 4 pages
    virt_addr_t f_TestVirt = VioletVmm_AllocatePages(f_KernelSpace, 4, VMM_FLAGS_KERNEL_DATA);
    VIOLET_PANIC_IF(f_TestVirt == 0, "VMM stress: failed to allocate 4 pages");

    // write a pattern across all 4 pages
    uint32_t* f_TestMem = (uint32_t*)f_TestVirt;
    for (size_t lv_Index = 0; lv_Index < (4 * VIOLET_PAGE_SIZE) / sizeof(uint32_t); lv_Index++)
    {
        f_TestMem[lv_Index] = (uint32_t)(0xC0FFEE00 + lv_Index);
    }

    // read it back and verify
    for (size_t lv_Index = 0; lv_Index < (4 * VIOLET_PAGE_SIZE) / sizeof(uint32_t); lv_Index++)
    {
        VIOLET_PANIC_IF(f_TestMem[lv_Index] != (uint32_t)(0xC0FFEE00 + lv_Index), "VMM stress: memory corruption detected");
    }

    // free and verify the PMM got the pages back
    uint64_t f_FreeBeforeAlloc = VioletPmm_GetFreePageCount();
    VioletVmm_FreePages(f_KernelSpace, f_TestVirt);
    VIOLET_PANIC_IF(VioletPmm_GetFreePageCount() != f_FreeBeforeAlloc + 4, "VMM stress: PMM free page count wrong after free");

    // allocate again — should succeed immediately
    virt_addr_t f_TestVirt2 = VioletVmm_AllocatePages(f_KernelSpace, 4, VMM_FLAGS_KERNEL_DATA);
    VIOLET_PANIC_IF(f_TestVirt2 == 0, "VMM stress: failed to allocate again after free");
    VioletVmm_FreePages(f_KernelSpace, f_TestVirt2);

    VioletGopConsole_PrintLine(&f_VioletConsole, "VMM stress test passed 💜");

    // VioletArch_SleepCycles(10'000'000'000);

    // VIOLET_PANIC_IF(true, "Test UwU!");

    //////////////////////////////////////// Kernel Main Loop ////////////////////////////////////////

    size_t f_Counter = 0;

    for (;;)
    {
        VioletArch_SleepCycles(1'000'000'000);

        VioletGopConsole_PrintLine(&f_VioletConsole, uintn_to_str(f_Counter++));        
    }
}
