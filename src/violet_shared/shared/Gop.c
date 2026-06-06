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
#include "Gop.h"


/*============================================================
    VioletInitGop
==============================================================*/

EFI_STATUS 
    VioletInitGop
    (
        EFI_SYSTEM_TABLE* fp_SystemTable, 
        VioletFrameBuffer* fp_OutFrameBuffer
    )
{
    EFI_GUID f_GopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* f_Gop = NULL;

    EFI_STATUS f_Status = fp_SystemTable->BootServices->LocateProtocol
    (
        &f_GopGuid,
        NULL,
        (VOID**)&f_Gop
    );

    if (EFI_ERROR(f_Status))
    {
        return f_Status;
    }

    // use whatever mode is already set — can query/set modes later
    fp_OutFrameBuffer->FrameBufferBase    = f_Gop->Mode->FrameBufferBase;
    fp_OutFrameBuffer->Width              = f_Gop->Mode->Info->HorizontalResolution;
    fp_OutFrameBuffer->Height             = f_Gop->Mode->Info->VerticalResolution;
    fp_OutFrameBuffer->PixelsPerScanLine  = f_Gop->Mode->Info->PixelsPerScanLine;

    return EFI_SUCCESS;
}

/*============================================================
    VioletDrawPixel
==============================================================*/

void 
    VioletDrawPixel
    (
        const VioletFrameBuffer* fp_FrameBuffer,
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
        row stride is PixelsPerScanLine NOT Width — rows may be padded by hardware
        casting VioletColour* to uint32_t* is safe:
            - VioletColour is exactly 4 bytes (4x uint8_t, no padding by design)
            - uint32_t* read gives us the packed BGRX value the hardware expects
    */
    uint32_t* f_Fb = (uint32_t*)fp_FrameBuffer->FrameBufferBase;
    f_Fb[fp_Y * fp_FrameBuffer->PixelsPerScanLine + fp_X] = *(uint32_t*)&fp_Colour;
}

/*============================================================
    VioletFillRect
==============================================================*/

void 
    VioletFillRect
    (
        const VioletFrameBuffer* fp_FrameBuffer,
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
    VioletClearScreen
    (
        const VioletFrameBuffer* fp_FrameBuffer,
        VioletColour             fp_Colour
    )
{
    VioletFillRect(fp_FrameBuffer, 0, 0, fp_FrameBuffer->Width, fp_FrameBuffer->Height, fp_Colour);
}