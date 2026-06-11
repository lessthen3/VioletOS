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
#ifndef VIOLET_SHARED_GOP_FRAME_BUFFER_HG
#define VIOLET_SHARED_GOP_FRAME_BUFFER_HG

///CSTD
#include <stdint.h>
#include <iso646.h>

///VioletShared
#include "VioletColour.h"

/*
    must only use fixed-width types here; 
    this struct is read by the kernel which has no UEFI headers, so no UINTN, no EFI_MEMORY_DESCRIPTOR, etc;
    everything must be plain C types that mean the same thing in both contexts
*/

typedef struct {
    uint64_t FrameBufferBase;
    uint32_t Width;
    uint32_t Height;
    uint32_t PixelsPerScanLine;
    uint32_t PixelFormat;         /* 0=RGBx 1=BGRx, matches EFI_GRAPHICS_PIXEL_FORMAT */
} VioletGop_FrameBuffer;

/*============================================================
    VioletGopFrameBuffer_DrawPixel
==============================================================*/

void 
    VioletGopFrameBuffer_DrawPixel
    (
        const VioletGop_FrameBuffer* fp_FrameBuffer,
        uint32_t                     fp_X,
        uint32_t                     fp_Y,
        VioletGop_Colour             fp_Colour
    );

/*============================================================
    VioletGopFrameBuffer_FillRect
==============================================================*/

void 
    VioletGopFrameBuffer_FillRect
    (
        const VioletGop_FrameBuffer* fp_FrameBuffer,
        uint32_t                     fp_X,
        uint32_t                     fp_Y,
        uint32_t                     fp_Width,
        uint32_t                     fp_Height,
        VioletGop_Colour             fp_Colour
    );

/*============================================================
    VioletGopFrameBuffer_ClearScreen
==============================================================*/

void 
    VioletGopFrameBuffer_ClearScreen
    (
        const VioletGop_FrameBuffer* fp_FrameBuffer,
        VioletGop_Colour             fp_Colour
    );

/*============================================================
    VioletGopFrameBuffer_ShiftAllPixels
==============================================================*/

void
    VioletGopFrameBuffer_ShiftAllPixelsVertical
    (
        const VioletGop_FrameBuffer* fp_FrameBuffer,
        int                          fp_Offset,
        VioletGop_Colour             fp_ExposedColour
    );

#endif /*VIOLET_SHARED_GOP_FRAME_BUFFER_HG*/