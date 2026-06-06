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
#ifndef VIOLET_BOOTLOADER_GOP_HG
#define VIOLET_BOOTLOADER_GOP_HG

#include <stdint.h>
#include <iso646.h>

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>

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
    uint64_t FrameBufferBase;     // physical address of pixel buffer
    uint32_t Width;
    uint32_t Height;
    uint32_t PixelsPerScanLine;   // NOT always == Width — rows may be padded
    EFI_GRAPHICS_PIXEL_FORMAT   PixelFormat;
} VioletFrameBuffer;

typedef struct {    //little endian ig
    uint8_t Blue;
    uint8_t Green;
    uint8_t Red;
    uint8_t Garbage; //mainly just padding, idk if an alpha channel can be used from uefi; memory aligned accesses babbby
} VioletColour;

/*============================================================
    VioletInitGop
==============================================================*/

EFI_STATUS 
    VioletInitGop
    (
        EFI_SYSTEM_TABLE* fp_SystemTable, 
        VioletFrameBuffer* fp_OutFrameBuffer
    );


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
    );

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
    );

/*============================================================
    VioletClearScreen
==============================================================*/

void 
    VioletClearScreen
    (
        const VioletFrameBuffer* fp_FrameBuffer,
        VioletColour             fp_Colour
    );

#endif /*VIOLET_BOOTLOADER_GOP_HG*/

