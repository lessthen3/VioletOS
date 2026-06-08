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
    uint32_t* f_Fb          = (uint32_t*)fp_FrameBuffer->FrameBufferBase;

    for (uint32_t fv_Y = fp_Y; fv_Y < f_EndY; fv_Y++)
    {
        /*
            compute row base once per row instead of doing the full multiply
            inside the inner loop — small but real optimization since this
            runs for every pixel in potentially a fullscreen clear
        */
        uint32_t* f_Row = f_Fb + fv_Y * fp_FrameBuffer->PixelsPerScanLine;

        for (uint32_t fv_X = fp_X; fv_X < f_EndX; fv_X++)
        {
            f_Row[fv_X] = f_PackedColour;
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