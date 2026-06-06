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

#include "shared/Gop.h"

typedef struct {
    const VioletFrameBuffer* Framebuffer;
    uint32_t                 CursorX;
    uint32_t                 CursorY;
    VioletColour             ForegroundColour;
    VioletColour             BackgroundColour;
} VioletConsole;

/*============================================================
    VioletConsoleInit: initialise a console over a framebuffer cursor starts at (0, 0), colours default to white on black
==============================================================*/

VioletConsole 
    VioletConsole_Create
    (
        const VioletFrameBuffer* fp_Framebuffer,
        VioletColour             fp_ForegroundColour,
        VioletColour             fp_BackgroundColour
    );

/*============================================================
    VioletConsolePutChar: draw one character at the current cursor position advances cursor, wraps at right edge, TODO: scrolls at bottom
==============================================================*/

void 
    VioletConsole_PutChar
    (
        VioletConsole* fp_Console,
        char           fp_Char
    );

/*============================================================
    VioletConsolePrint: draw a null terminated string
==============================================================*/

void 
    VioletConsole_Print
    (
        VioletConsole* fp_Console,
        const char*    fp_String
    );

/*============================================================
    VioletConsolePrintLine: draw a string followed by a newline
==============================================================*/

void 
    VioletConsole_PrintLine
    (
        VioletConsole* fp_Console,
        const char*    fp_String
    );

/*============================================================
    VioletConsoleInit
==============================================================*/


#endif /*VIOLET_KERNEL_GOP_CONSOLE_HG*/