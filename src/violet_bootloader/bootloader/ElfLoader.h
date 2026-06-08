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

/// VioletOS
#include "shared/gop/console/GopConsole.h"

/*
    describes first 64 bytes of any 64-bit ELF file;
    we only read what we actually need for loading
*/
typedef struct {
    uint8_t  Ident[16];     /* magic + class + endianness + version + padding */
    uint16_t Type;          /* ET_EXEC = 2 (executable), ET_DYN = 3 (PIE) */
    uint16_t Machine;       /* EM_X86_64 = 0x3E */
    uint32_t Version;
    uint64_t EntryPoint;    /* virtual address of entry point — this is KernelMain */
    uint64_t ProgramHeaderOffset;      /* file offset to first program header */
    uint64_t ShOffset;      /* file offset to first section header — we don't use this */
    uint32_t Flags;
    uint16_t EhSize;        /* size of this header, should be 64 */
    uint16_t ProgramHeaderEntrySize;   /* size of one program header entry */
    uint16_t ProgramHeaderCount;       /* number of program headers */
    uint16_t ShEntrySize;
    uint16_t ShCount;
    uint16_t ShStrIndex;
} Elf64Header;

/*
    describes one loadable segment in an elf64
*/

typedef struct {
    uint32_t Type;          /* PT_LOAD = 1 — the only type we care about */
    uint32_t Flags;         /* PF_X=1 PF_W=2 PF_R=4 — execute/write/read */
    uint64_t FileOffset;    /* offset into ELF file where segment data starts */
    uint64_t VirtualAddr;   /* virtual address to map this segment to */
    uint64_t PhysicalAddr;  /* physical address hint — we use VirtualAddr */
    uint64_t FileSize;      /* bytes in the file — copy this many bytes */
    uint64_t MemorySize;    /* bytes in memory — may be larger than FileSize (BSS) */
    uint64_t Alignment;     /* segment alignment requirement */
} Elf64ProgramHeader;

/* 
    ELF magic bytes, first 4 bytes are the identifier, the next few bytes are configs
*/

#define ELF_MAGIC_0  0x7F // '.'
#define ELF_MAGIC_1  0x45 // 'E'
#define ELF_MAGIC_2  0x4C // 'L'
#define ELF_MAGIC_3  0x46 // 'F'

#define ELF_CLASS_64        2       /* Ident[4] — 64-bit ELF */
#define ELF_DATA_LSB        1       /* Ident[5] — little-endian */
#define ELF_TYPE_EXEC       2       /* executable */
#define ELF_MACHINE_X86_64  0x3E    /* x86-64 */
#define ELF_PT_LOAD         1       /* loadable segment */

/*
    EntryPoint: virtual address of KernelMain, cast to function pointer and called after ExitBootServices + page table setup
*/

typedef struct {
    uint64_t EntryPoint;    /* e_entry from ELF header — where to jump */
    uint64_t LoadBase;      /* lowest physical address we loaded any segment to */
    uint64_t LoadEnd;       /* highest physical address used — for PMM to avoid */
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
        VioletConsole*     fp_VioletConsole
    );


#endif /*VIOLET_BOOTLOADER_ELF_LOADER_HG*/

