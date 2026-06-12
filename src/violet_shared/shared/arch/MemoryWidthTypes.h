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
#ifndef VIOLET_SHARED_ARCH_MEMORY_WIDTH_TYPES_HG
#define VIOLET_SHARED_ARCH_MEMORY_WIDTH_TYPES_HG

#include <stdint.h>
#include <stddef.h>

typedef uint64_t    phys_addr_t; // Physical addresses are always 64-bit (handles x86_64 and 32-bit PAE)
typedef uintptr_t   virt_addr_t; // Virtual addresses match the native CPU register width

#define VIOLET_PHYSICAL_ADDRESS_MAX  UINT64_MAX
#define VIOLET_VIRTUAL_ADDRESS_MAX   UINTPTR_MAX


#define VIOLET_PAGE_SIZE ((phys_addr_t)0x1000)

#endif /*VIOLET_SHARED_ARCH_MEMORY_WIDTH_TYPES_HG*/
