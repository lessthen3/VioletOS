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
#ifndef VIOLET_KERNEL_GOP_CONSOLE_HG
#define VIOLET_KERNEL_GOP_CONSOLE_HG

#include "shared/gop/GopFrameBuffer.h"

typedef struct {
    const VioletGop_FrameBuffer* Framebuffer;
    uint32_t                 CursorX;
    uint32_t                 CursorY;
    VioletGop_Colour             ForegroundColour;
    VioletGop_Colour             BackgroundColour;
} VioletGop_Console;

/*============================================================
    VioletGopConsole_Create: initialise a console over a framebuffer cursor starts at (0, 0), colours default to white on black
==============================================================*/

VioletGop_Console 
    VioletGopConsole_Create
    (
        const VioletGop_FrameBuffer* fp_Framebuffer,
        VioletGop_Colour             fp_ForegroundColour,
        VioletGop_Colour             fp_BackgroundColour
    );

/*============================================================
    VioletGopConsole_PutChar: draw one character at the current cursor position advances cursor, wraps at right edge, TODO: scrolls at bottom
==============================================================*/

void 
    VioletGopConsole_PutChar
    (
        VioletGop_Console* fp_Console,
        char           fp_Char
    );

/*============================================================
    VioletGopConsole_Print: draw a null terminated string
==============================================================*/

void 
    VioletGopConsole_Print
    (
        VioletGop_Console* fp_Console,
        const char*    fp_String
    );

/*============================================================
    VioletGopConsole_PrintLine: draw a string followed by a newline
==============================================================*/

void 
    VioletGopConsole_PrintLine
    (
        VioletGop_Console* fp_Console,
        const char*    fp_String
    );

/*============================================================
    VioletGopConsole_DrawBlockCursor
==============================================================*/

void 
    VioletGopConsole_DrawBlockCursor
    (
        VioletGop_Console* fp_Console, 
        VioletGop_Colour fp_Colour
    );

#endif /*VIOLET_KERNEL_GOP_CONSOLE_HG*/