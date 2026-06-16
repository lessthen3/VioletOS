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
#ifndef VIOLET_BOOTLOADER_ELF_LOADER_HG
#define VIOLET_BOOTLOADER_ELF_LOADER_HG

/// CSTD
#include <stdint.h>

/// UEFI
#include <Uefi.h>

/// VioletShared
#include "shared/gop/GopConsole.h"
#include "shared/arch/Memory.h"

/*
    EntryPoint: virtual address of KernelMain, cast to function pointer and called after ExitBootServices + page table setup
*/

typedef struct {
    virt_addr_t EntryPoint;    /* e_entry from ELF header — where to jump */
    phys_addr_t LoadBase;      /* lowest physical address we loaded any segment to */
    phys_addr_t LoadEnd;       /* highest physical address used — for PMM to avoid */
} VioletLoadedKernel;

/*
    VioletLoadKernelElf: open violet_kernel.elf from the ESP, parse program headers,
    copy PT_LOAD segments to their virtual addresses, zero BSS padding

    fp_ImageHandle: your UefiMain fp_ImageHandle — needed to get the filesystem protocol
    fp_SystemTable: for BootServices->AllocatePages and filesystem access
    fp_OutKernel:   populated on success with entry point and load addresses

    returns EFI_SUCCESS on success, EFI error code otherwise;
    on failure the console should print an error, caller decides whether to hang or retry
*/

EFI_STATUS
    Violet_LoadKernelElf
    (
        EFI_HANDLE         fp_ImageHandle,
        EFI_SYSTEM_TABLE*  fp_SystemTable,
        VioletLoadedKernel* fp_OutKernel,
        VioletGop_Console*     fp_VioletConsole
    );


#endif /*VIOLET_BOOTLOADER_ELF_LOADER_HG*/

