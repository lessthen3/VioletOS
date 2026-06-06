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
#ifndef VIOLET_KERNEL_PANIC_HG
#define VIOLET_KERNEL_PANIC_HG

#include "GeneralMacros.h"

/*
    forward declare — implemented in Panic.c once we have serial/framebuffer output
    VIOLET_NORETURN tells the compiler this function never returns
    so it doesnt generate code to handle a return path after the call
    and doesnt warn about missing returns in functions that call this
*/

[[noreturn]] void 
    VioletPanicExit
    (
        const char* fp_Condition, 
        const char* fp_Message
    );


#define VIOLET_PANIC_IF(fp_Condition, fp_Message) (VIOLET_UNLIKELY(fp_Condition) ? VioletPanicExit(#fp_Condition, fp_Message) : (void)0)

/*
    VIOLET_ASSERT — debug-only panic, stripped in release builds
    for catching invariant violations during development
*/
#ifdef NDEBUG
#   define VIOLET_ASSERT(fp_Condition, fp_Message) ((void)0)
#else
#   define VIOLET_ASSERT(fp_Condition, fp_Message) VIOLET_PANIC_IF(!(fp_Condition), fp_Message)
#endif

#endif /*VIOLET_KERNEL_PANIC_HG*/

