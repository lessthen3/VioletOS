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

#include "VioletColour.h"
#include "shared/gop/GopFrameBuffer.h"

#define VIOLET_COLOUR(fp_R, fp_G, fp_B) ((VioletColour){ .Blue = (fp_B), .Green = (fp_G), .Red = (fp_R), .Garbage = 0 })

#define VIOLET_COLOUR_VIOLET  VIOLET_COLOUR(0x8d, 0x21, 0x86)

#define VIOLET_COLOUR_BLACK   VIOLET_COLOUR(0,   0,   0  )
#define VIOLET_COLOUR_WHITE   VIOLET_COLOUR(255, 255, 255)
#define VIOLET_COLOUR_RED     VIOLET_COLOUR(255, 0,   0  )
#define VIOLET_COLOUR_GREEN   VIOLET_COLOUR(0,   255, 0  )
#define VIOLET_COLOUR_BLUE    VIOLET_COLOUR(0,   0,   255)
// #define VIOLET_COLOUR_VIOLET  VIOLET_COLOUR(148, 0,   211)  /* 💜 */
#define VIOLET_COLOUR_CYAN    VIOLET_COLOUR(0,   255, 255)
#define VIOLET_COLOUR_YELLOW  VIOLET_COLOUR(255, 255, 0  )

// #8d2186

// #df4999
// #a41da7

typedef struct {
    const VioletGop_FrameBuffer* Framebuffer;
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
        const VioletGop_FrameBuffer* fp_Framebuffer,
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