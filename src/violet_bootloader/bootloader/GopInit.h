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
#ifndef VIOLET_BOOTLOADER_GOP_INIT_HG
#define VIOLET_BOOTLOADER_GOP_INIT_HG

///CSTD
#include <stdint.h>
#include <iso646.h>

///UEFI
#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>

///VioletShared
#include "shared/gop/GopFrameBuffer.h"

/*============================================================
    VioletInitGop
==============================================================*/

EFI_STATUS 
    Violet_InitGop
    (
        EFI_SYSTEM_TABLE* fp_SystemTable, 
        VioletGop_FrameBuffer* fp_OutFrameBuffer
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

#endif /*VIOLET_BOOTLOADER_GOP_INIT_HG*/

