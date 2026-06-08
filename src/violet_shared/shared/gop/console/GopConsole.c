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
#include "GopConsole.h"
#include "VioletFontData.h"

#include <iso646.h>

/*============================================================
    VioletConsoleInit
==============================================================*/

VioletConsole 
    VioletConsole_Create
    (
        const VioletGop_FrameBuffer* fp_FrameBuffer,
        VioletColour             fp_ForegroundColour,
        VioletColour             fp_BackgroundColour
    )
{
    if(not fp_FrameBuffer)
    {
        //TODO_ERROR
    }

    VioletConsole f_VioletConsole;

    f_VioletConsole.Framebuffer        = fp_FrameBuffer;
    f_VioletConsole.CursorX            = 0;
    f_VioletConsole.CursorY            = 0;
    f_VioletConsole.ForegroundColour   = fp_ForegroundColour;
    f_VioletConsole.BackgroundColour   = fp_BackgroundColour;

    return f_VioletConsole;
}

/*============================================================
    VioletConsolePutChar
==============================================================*/

void 
    VioletConsole_PutChar
    (
        VioletConsole* fp_Console,
        char           fp_Char
    )
{
    const VioletGop_FrameBuffer* f_Framebuffer = fp_Console->Framebuffer;

    if (fp_Char == '\n')
    {
        fp_Console->CursorX  = 0;
        fp_Console->CursorY += 1;

        return;
    }

    if (fp_Char == '\r')
    {
        fp_Console->CursorX = 0;
        
        return;
    }

    /*
        clamp character to printable ASCII range
        anything outside [0x20, 0x7F] we just render as space for now
    */
    uint8_t f_GlyphIndex = (uint8_t)fp_Char; //this should be safe but w/e

    if (f_GlyphIndex < 0x20 || f_GlyphIndex > 0x7F)
    {
        f_GlyphIndex = 0x20;
    }

    /*
        glyph bitmap starts at VioletFont8x16[glyphIndex * VIOLET_FONT_HEIGHT]
        each of the 16 bytes encodes one row of 8 pixels
        bit 7 of each byte = leftmost pixel of that row
    */
    const uint8_t* f_Glyph     = &VioletFontSpleen8x16[f_GlyphIndex * VIOLET_FONT_HEIGHT];
    uint32_t       f_PixelX    = fp_Console->CursorX * VIOLET_FONT_WIDTH;
    uint32_t       f_PixelY    = fp_Console->CursorY * VIOLET_FONT_HEIGHT;

    uint32_t f_PackedForeground = *(uint32_t*)&fp_Console->ForegroundColour;
    uint32_t f_PackedBackground = *(uint32_t*)&fp_Console->BackgroundColour;
    uint32_t* f_FramebufferBase = (uint32_t*)f_Framebuffer->FrameBufferBase;

    for (uint32_t fv_Row = 0; fv_Row < VIOLET_FONT_HEIGHT; fv_Row++)
    {
        uint8_t  f_RowBits = f_Glyph[fv_Row];
        uint32_t* f_Row    = f_FramebufferBase + (f_PixelY + fv_Row) * f_Framebuffer->PixelsPerScanLine;

        for (uint32_t fv_Column = 0; fv_Column < VIOLET_FONT_WIDTH; fv_Column++)
        {
            /*
                check if this pixel is set in the glyph bitmap
                bit 7 = leftmost column, so shift right by (7 - column)
            */
            f_Row[f_PixelX + fv_Column] = (f_RowBits >> (7 - fv_Column)) & 1
                ? f_PackedForeground
                : f_PackedBackground;
        }
    }

    /* advance cursor, wrap at right edge */
    fp_Console->CursorX += 1;

    uint32_t f_ColumnsPerRow = f_Framebuffer->Width / VIOLET_FONT_WIDTH;

    if (fp_Console->CursorX >= f_ColumnsPerRow)
    {
        fp_Console->CursorX  = 0;
        fp_Console->CursorY += 1;
    }

    /*
        TODO: scroll when CursorY * VIOLET_FONT_HEIGHT >= Framebuffer->Height
        for now we just let it go off screen
    */
}

/*============================================================
    VioletConsolePrint
==============================================================*/

void 
    VioletConsole_Print
    (
        VioletConsole* fp_Console,
        const char*    fp_String
    )
{
    while (*fp_String != '\0')
    {
        VioletConsole_PutChar(fp_Console, *fp_String);
        fp_String++;
    }
}

/*============================================================
    VioletConsolePrintLine
==============================================================*/
void 
    VioletConsole_PrintLine
    (
        VioletConsole* fp_Console,
        const char*    fp_String
    )
{
    VioletConsole_Print(fp_Console, fp_String);
    VioletConsole_PutChar(fp_Console, '\n');
}
