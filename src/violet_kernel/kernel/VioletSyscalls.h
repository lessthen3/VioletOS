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
#ifndef VIOLET_KERNEL_SYSCALLS_HG
#define VIOLET_KERNEL_SYSCALLS_HG

#include "shared/GeneralMacros.h"
#include "shared/SyscallNumbers.h"

#include <stdint.h>


static VIOLET_INLINE int64_t
    VioletSyscall_IMPLEMENTATION(uint64_t fp_Number, uint64_t fp_Arg0, uint64_t fp_Arg1, uint64_t fp_Arg2)
{
    int64_t f_Result;

    __asm__ volatile 
    (
        "syscall"
        : "=a" (f_Result)                              // output: rax → f_Result
        : "0"  (fp_Number),                            // input:  fp_Number → rax
          "D"  (fp_Arg0),                              // rdi
          "S"  (fp_Arg1),                              // rsi
          "d"  (fp_Arg2)                               // rdx
        : "rcx", "r11", "memory"                       // syscall clobbers rcx and r11
    );

    return f_Result;
}

// then you build ergonomic wrappers on top:
static VIOLET_INLINE int64_t 
    VioletSyscall_SendMessage(uint64_t fp_Endpoint, void* fp_Msg, uint64_t fp_Len)
{
    return VioletSyscall_IMPLEMENTATION(VIOLET_SYSCALL_SEND, fp_Endpoint, (uint64_t)fp_Msg, fp_Len);
}

static VIOLET_INLINE int64_t
    VioletSyscall_ReceiveMessage(uint64_t fp_Endpoint, void* fp_Buf, uint64_t fp_BufLen)
    {
        return VioletSyscall_IMPLEMENTATION(VIOLET_SYSCALL_RECV, fp_Endpoint, (uint64_t)fp_Buf, fp_BufLen);
    }

static VIOLET_INLINE int64_t
    VioletSyscall_Exit(int64_t fp_Code)
    {
        return VioletSyscall_IMPLEMENTATION(VIOLET_SYSCALL_EXIT, (uint64_t)fp_Code, 0, 0);
    }

static VIOLET_INLINE int64_t
    VioletSyscall_Yield()
    {
        return VioletSyscall_IMPLEMENTATION(VIOLET_SYSCALL_YIELD, 0, 0, 0);
    }

#endif /*VIOLET_KERNEL_SYSCALLS_HG*/

