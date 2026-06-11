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
#include "VioletPanic.h"

///VioletShared
#include "shared/gop/console/GopConsole.h"
#include "shared/arch/DisableInterrupts.h"

///CSTD
#include <iso646.h>
#include <stddef.h>

/*============================================================
    static panic console — set once by VioletPanic_Init
    all panics after that use this to print
    if panic fires before init: no output, just halt
    better than triple faulting on a null dereference
==============================================================*/

static VioletGop_FrameBuffer* s_GopFrameBuffer = NULL;

/*============================================================
    VioletPanic_Init
==============================================================*/

void 
    VioletPanic_Init(VioletGop_FrameBuffer* fp_FrameBuffer)
{
    if(not fp_FrameBuffer)
    {

        return; //idek what to do here lmfao
    }

    s_GopFrameBuffer = fp_FrameBuffer;
}


/*============================================================
    VioletPanicExit
==============================================================*/

void
    VioletPanicExit
    (
        const char* fp_Condition,
        const char* fp_Message,
        const char* fp_Location
    )
{
    /*
        disable interrupts immediately — nothing preempts a panic
        CLI is a privileged instruction, valid in ring 0
        after this no IRQ can fire and context-switch us away
    */
    VIOLET_DISABLE_INTERRUPTS;

    if (s_GopFrameBuffer not_eq NULL) //this is so we don't triple fault, at least the previous frame buffer will be there and just freeze
    {
        /*
            paint the screen red so it's instantly recognisable as a panic
            even if text rendering somehow fails you still see red
        */
        VioletGopFrameBuffer_ClearScreen(s_GopFrameBuffer, VIOLET_COLOUR_RED);

        /*
            reset cursor and switch to white-on-red for panic text
            don't modify the caller's console — write directly to a local copy
            since we're never returning anyway it doesn't matter
        */
        VioletGop_Console f_PanicConsole = VioletGopConsole_Create
        (
            s_GopFrameBuffer,
            VIOLET_COLOUR_WHITE, 
            VIOLET_COLOUR_RED
        );

        VioletGopConsole_PrintLine(&f_PanicConsole, "");
        VioletGopConsole_PrintLine(&f_PanicConsole, "  !! KERNEL PANIC !!");
        VioletGopConsole_PrintLine(&f_PanicConsole, "");

        if (fp_Condition not_eq NULL)
        {
            VioletGopConsole_Print(&f_PanicConsole, "  Condition Failed: ");
            VioletGopConsole_PrintLine(&f_PanicConsole, fp_Condition);
        }

        if (fp_Message not_eq NULL)
        {
            VioletGopConsole_Print(&f_PanicConsole, "  What Happened:   ");
            VioletGopConsole_PrintLine(&f_PanicConsole, fp_Message);
        }

        if(fp_Location not_eq NULL)
        {
            VioletGopConsole_Print(&f_PanicConsole, "  Location:   ");
            VioletGopConsole_PrintLine(&f_PanicConsole, fp_Location);
        }

        VioletGopConsole_PrintLine(&f_PanicConsole, "");
        VioletGopConsole_PrintLine(&f_PanicConsole, "  system halted.");
    }

    /*
        halt forever — HLT puts the CPU in a low-power wait state
        interrupts are disabled so nothing can wake it
        the loop is a safety net in case something re-enables interrupts
        before halting (shouldn't happen but defensive is correct here)
    */
    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}