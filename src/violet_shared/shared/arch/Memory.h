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
#ifndef VIOLET_SHARED_ARCH_MEMORY_HG
#define VIOLET_SHARED_ARCH_MEMORY_HG

#include <stdint.h>

typedef uint64_t    phys_addr_t; // Physical addresses are always 64-bit (handles x86_64 and 32-bit PAE)
typedef uintptr_t   virt_addr_t; // Virtual addresses match the native CPU register width

#define VIOLET_PHYSICAL_ADDRESS_MAX  UINT64_MAX
#define VIOLET_VIRTUAL_ADDRESS_MAX   UINTPTR_MAX

#define VIOLET_PAGE_SIZE ((phys_addr_t)0x1000)

/*============================================================
    Virtual Address Space Layout

    0x0000000000001000 — 0x00007FFFFFFFFFFF   user space (128TB)
    0xFFFF800000000000 — 0xFFFFBFFFFFFFFFFF   direct physical map (64TB) -- every byte of physical RAM is accessible at DIRECT_MAP_BASE + phys_addr
    0xFFFFFF8000000000 — 0xFFFFFFFFFFFFFFFF   kernel virtual space
    0xFFFFFFFF80000000                        kernel image base (VMA in the linker script)
==============================================================*/

constexpr virt_addr_t VIOLET_USER_SPACE_START    = (virt_addr_t)0x0000000000001000;
constexpr virt_addr_t VIOLET_USER_SPACE_END      = (virt_addr_t)0x00007FFFFFFFF000;

constexpr virt_addr_t VIOLET_DIRECT_MAP_BASE     = (virt_addr_t)0xFFFF800000000000;
constexpr virt_addr_t VIOLET_DIRECT_MAP_END      = (virt_addr_t)0xFFFFBFFFFFFFFFFF;

constexpr virt_addr_t VIOLET_KERNEL_SPACE_START  = (virt_addr_t)0xFFFFFF8000000000;
constexpr virt_addr_t VIOLET_KERNEL_IMAGE_BASE   = (virt_addr_t)0xFFFFFFFF80000000;

#endif /*VIOLET_SHARED_ARCH_MEMORY_HG*/
