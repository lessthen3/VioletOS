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

#include "shared/gop/GopFrameBuffer.h"

/*
    forward declare — implemented in Panic.c once we have serial/framebuffer output
    VIOLET_NORETURN tells the compiler this function never returns
    so it doesnt generate code to handle a return path after the call
    and doesnt warn about missing returns in functions that call this
*/

void 
    VioletPanic_Init(VioletGop_FrameBuffer* fp_FrameBuffer);

[[noreturn]] void 
    VioletPanicExit
    (
        const char* fp_Condition, 
        const char* fp_Message,
        const char* fp_FileName,
        const char* fp_FunctionName
    );

//================================================================================ File Name OwO ================================================================================//

#define VIOLET_FILENAME ((__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__))

//================================================================================ Panic ;w; ================================================================================//

#define VIOLET_PANIC_IF(fp_Condition, fp_Message) (VIOLET_UNLIKELY(fp_Condition) ? VioletPanicExit(#fp_Condition, fp_Message, VIOLET_FILENAME, __PRETTY_FUNCTION__) : (void)0)

//================================================================================ Assert >w< ================================================================================//

#ifdef NDEBUG
#   define VIOLET_ASSERT(fp_Condition, fp_Message) ((void)0)
#else
#   define VIOLET_ASSERT(fp_Condition, fp_Message) VIOLET_PANIC_IF(!(fp_Condition), fp_Message)
#endif

#endif /*VIOLET_KERNEL_PANIC_HG*/

