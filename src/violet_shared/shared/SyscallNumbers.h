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
#ifndef VIOLET_SHARED_SYSCALL_NUMBERS_HG
#define VIOLET_SHARED_SYSCALL_NUMBERS_HG

#define VIOLET_SYSCALL_SEND     1u
#define VIOLET_SYSCALL_RECV     2u
#define VIOLET_SYSCALL_MAP      3u
#define VIOLET_SYSCALL_UNMAP    4u
#define VIOLET_SYSCALL_SPAWN    5u
#define VIOLET_SYSCALL_EXIT     6u
#define VIOLET_SYSCALL_YIELD    7u   // voluntarily yield timeslice
#define VIOLET_SYSCALL_CALL     8u   // send + block for reply (common pattern, worth making atomic)

#endif /*VIOLET_SHARED_SYSCALL_NUMBERS_HG*/