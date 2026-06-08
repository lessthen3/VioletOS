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
#include "GopFrameBuffer.h"

/*============================================================
    VioletDrawPixel
==============================================================*/

void 
    VioletGopFrameBuffer_DrawPixel
    (
        const VioletGop_FrameBuffer* fp_FrameBuffer,
        uint32_t                 fp_X,
        uint32_t                 fp_Y,
        VioletColour             fp_Colour
    )
{
    if(not fp_FrameBuffer)
    {
        //TODO_ERROR
    }

    /*
        framebuffer is a flat array of 32-bit pixels
        row stride is PixelsPerScanLine NOT Width; rows may be padded by hardware;
        casting VioletColour* to uint32_t* is safe:
            - VioletColour is exactly 4 bytes
            - uint32_t* read gives us the packed BGRX value the hardware expects
    */
    uint32_t* f_Fb = (uint32_t*)fp_FrameBuffer->FrameBufferBase;
    f_Fb[fp_Y * fp_FrameBuffer->PixelsPerScanLine + fp_X] = *(uint32_t*)&fp_Colour;
}

/*============================================================
    VioletFillRect
==============================================================*/

void 
    VioletGopFrameBuffer_FillRect
    (
        const VioletGop_FrameBuffer* fp_FrameBuffer,
        uint32_t                 fp_X,
        uint32_t                 fp_Y,
        uint32_t                 fp_Width,
        uint32_t                 fp_Height,
        VioletColour             fp_Colour
    )
{
    if(not fp_FrameBuffer) //unlikely here uwu
    {
        //TODO_ERROR
    }

    /*
        clamp rect to framebuffer bounds so callers dont have to
        if the rect is entirely outside the screen just bail
    */
    if (fp_X >= fp_FrameBuffer->Width or fp_Y >= fp_FrameBuffer->Height)
    {
        return;
    }

    uint32_t f_EndX = fp_X + fp_Width;
    uint32_t f_EndY = fp_Y + fp_Height;

    if (f_EndX > fp_FrameBuffer->Width)
    {
        f_EndX = fp_FrameBuffer->Width;
    }

    if (f_EndY > fp_FrameBuffer->Height)
    {
        f_EndY = fp_FrameBuffer->Height;
    }

    uint32_t  f_PackedColour = *(uint32_t*)&fp_Colour;
    uint32_t* f_FrameBufferBase = (uint32_t*)fp_FrameBuffer->FrameBufferBase;

    for (uint32_t fv_RowNumber = fp_Y; fv_RowNumber < f_EndY; fv_RowNumber++)
    {
        /*
            compute row base once per row instead of doing the full multiply inside the inner loop; 
            small but real optimization since this runs for every pixel in potentially a fullscreen clear
        */
        
        uint32_t* f_Row = f_FrameBufferBase + fv_RowNumber*fp_FrameBuffer->PixelsPerScanLine;

        for (uint32_t fv_ColumnNumber = fp_X; fv_ColumnNumber < f_EndX; fv_ColumnNumber++)
        {
            f_Row[fv_ColumnNumber] = f_PackedColour;
        }
    }
}

/*============================================================
    VioletClearScreen
==============================================================*/

void 
    VioletGopFrameBuffer_ClearScreen
    (
        const VioletGop_FrameBuffer* fp_FrameBuffer,
        VioletColour             fp_Colour
    )
{
    VioletGopFrameBuffer_FillRect(fp_FrameBuffer, 0, 0, fp_FrameBuffer->Width, fp_FrameBuffer->Height, fp_Colour);
}


/*============================================================
    VioletGopFrameBuffer_ShiftAllPixelsVertical
==============================================================*/

void
    VioletGopFrameBuffer_ShiftAllPixelsVertical
    (
        const VioletGop_FrameBuffer* fp_FrameBuffer,
        int                          fp_Offset,
        VioletColour                 fp_ExposedColour
    )
{
    if (not fp_FrameBuffer or fp_Offset == 0)
    {
        return;
    }

    uint32_t* f_FrameBufferBase     = (uint32_t*)fp_FrameBuffer->FrameBufferBase;
    uint32_t  f_Stride = fp_FrameBuffer->PixelsPerScanLine;
    uint32_t  f_Height = fp_FrameBuffer->Height;

    /*
        clamp offset to framebuffer height; 
        shifting more than the full height is just a clear
    */
    uint32_t f_AbsOffset = fp_Offset < 0 ? (uint32_t)(-fp_Offset) : (uint32_t)fp_Offset;

    if (f_AbsOffset >= f_Height)
    {
        VioletGopFrameBuffer_ClearScreen(fp_FrameBuffer, fp_ExposedColour);
        return;
    }

    uint32_t f_CopyRows   = f_Height - f_AbsOffset;
    uint32_t f_PackedFill = *(uint32_t*)&fp_ExposedColour;

    if (fp_Offset < 0)
    {
        /*
            shift UP; 
            content moves toward row 0;
            iterate top to bottom so we never read from a row we already wrote
        */

        for (uint32_t fv_Row = 0; fv_Row < f_CopyRows; fv_Row++)
        {
            uint32_t* f_Destination = f_FrameBufferBase + fv_Row*f_Stride;
            uint32_t* f_Source      = f_FrameBufferBase + (fv_Row + f_AbsOffset)*f_Stride;

            for (uint32_t fv_Col = 0; fv_Col < f_Stride; fv_Col++)
            {
                f_Destination[fv_Col] = f_Source[fv_Col];
            }
        }

        /* fill exposed rows at the bottom */

        for (uint32_t fv_Row = f_CopyRows; fv_Row < f_Height; fv_Row++)
        {
            uint32_t* f_Row = f_FrameBufferBase + fv_Row*f_Stride;

            for (uint32_t fv_Col = 0; fv_Col < f_Stride; fv_Col++)
            {
                f_Row[fv_Col] = f_PackedFill;
            }
        }
    }
    else
    {
        /*
            shift DOWN;
            content moves toward last row;
            iterate bottom to top so we never read from a row we already wrote
        */

        for (uint32_t fv_Row = f_CopyRows; fv_Row-- > 0;) // ah yes the approaches operator uwu
        {
            uint32_t* f_Destination = f_FrameBufferBase + (fv_Row + f_AbsOffset)*f_Stride;
            uint32_t* f_Source      = f_FrameBufferBase + fv_Row*f_Stride;

            for (uint32_t fv_Col = 0; fv_Col < f_Stride; fv_Col++)
            {
                f_Destination[fv_Col] = f_Source[fv_Col];
            }
        }

        /* fill exposed rows at the top */

        for (uint32_t fv_Row = 0; fv_Row < f_AbsOffset; fv_Row++)
        {
            uint32_t* f_Row = f_FrameBufferBase + fv_Row*f_Stride;

            for (uint32_t fv_Col = 0; fv_Col < f_Stride; fv_Col++)
            {
                f_Row[fv_Col] = f_PackedFill;
            }
        }
    }
}