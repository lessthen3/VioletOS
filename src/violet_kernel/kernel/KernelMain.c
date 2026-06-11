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
#include "shared/gop/console/GopConsole.h"
#include "shared/utils/StringConversion.h"
#include "shared/arch/Sleep.h"

///VioletKernel
#include "VioletPanic.h"
#include "kernel/memory/PhysicalMemoryManager.h"
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
        TODO: init VMM
        TODO: init GDT/IDT
        TODO: init LAPIC
        TODO: spawn root task
    */

__attribute__((section(".text.boot")))
void 
    KernelMain(VioletBoot_Info* fp_BootInfo)
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

    //////////////////////////////////////// Setup a console for now until we have a DE ////////////////////////////////////////

    VioletGopFrameBuffer_ClearScreen(&fp_BootInfo->FrameBuffer, VIOLET_COLOUR_GREEN);  // owo

    VioletGop_Console f_VioletConsole = VioletGopConsole_Create(&fp_BootInfo->FrameBuffer, VIOLET_COLOUR_WHITE, VIOLET_COLOUR_VIOLET);
    VioletGopConsole_PrintLine(&f_VioletConsole, "Hello World from the Kernel!");

    //////////////////////////////////////////////////////////////////////////////// Random Testing Stuff ////////////////////////////////////////////////////////////////////////////////

    // VIOLET_SLEEP_FOR_CYCLES(10'000'000'000);

    // VIOLET_PANIC_IF(true, "Test UwU!");

    //////////////////////////////////////// Kernel Main Loop ////////////////////////////////////////

    size_t f_Counter = 0;

    for (;;)
    {
        VIOLET_SLEEP_FOR_CYCLES(1'000'000'000);

        VioletGopConsole_PrintLine(&f_VioletConsole, uintn_to_str(f_Counter++));        
    }
}
