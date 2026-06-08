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

/*
    these symbols are defined by violet_kernel.ld, apparently better to use an array instead.
    ik it decays to a ptr but like apparently it's more correct than uint8_t* idfk

    future ryan: oh it's because its just a ptr, apparently uint8_t* alone will cause a dereference at some point
    for some reason lmfao idk, writing a compiler requires at least a few assumptions about memory layout i dont blame em
*/

extern uint8_t VioletTextStart[];
extern uint8_t VioletTextEnd[];

extern uint8_t VioletRodataStart[];
extern uint8_t VioletRodataStart[];

extern uint8_t VioletDataStart[];
extern uint8_t VioletDataEnd[];

extern uint8_t VioletBssStart[];
extern uint8_t VioletBssEnd[];

extern uint8_t VioletKernelEnd[];

/*
    VioletMemset: needed since we don't got libc
*/
static void 
    VioletMemset(void* fp_Destination, uint8_t fp_Value, size_t fp_Size)
{
    uint8_t* fv_Ptr = (uint8_t*)fp_Destination;

    for (size_t lv_I = 0; lv_I < fp_Size; lv_I++)
    {
        fv_Ptr[lv_I] = fp_Value;
    }
}


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

__attribute__((section(".text.boot")))
void 
    KernelMain(VioletBoot_Info* fp_BootInfo)
{
    /*
        FIRST THING: zero the BSS segment;
        crt0 doesnt exist so nobody else will do this;
        all uninitialized globals are garbage until we do this
    */
    VioletMemset(VioletBssStart, 0, (size_t)(VioletBssEnd - VioletBssStart));

    VioletGopFrameBuffer_ClearScreen(&fp_BootInfo->FrameBuffer, VIOLET_COLOUR_GREEN);  // owo

    VioletConsole f_VioletConsole = VioletConsole_Create(&fp_BootInfo->FrameBuffer, VIOLET_COLOUR_WHITE, VIOLET_COLOUR_VIOLET);
    VioletConsole_PrintLine(&f_VioletConsole, "Hello World from the Kernel!");
    
    /*
        TODO: accept BootInfo* parameter from bootloader once Entry.c is written
        TODO: init PMM from memory map
        TODO: init VMM
        TODO: init GDT/IDT
        TODO: init LAPIC
        TODO: spawn root task
    */
    
    /* spin forever until we have something to actually do */
    for (;;)
    {
        __asm__ volatile ("hlt");
        ;
    }
}
