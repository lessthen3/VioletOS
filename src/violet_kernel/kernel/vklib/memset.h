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
#ifndef VIOLET_KERNEL_VKLIB_MEMSET_HG
#define VIOLET_KERNEL_VKLIB_MEMSET_HG

///VioletShared
#include "shared/GeneralMacros.h"

///CSTD
#include <stdint.h>
#include <stddef.h>

/*
    VioletMemset: needed since we don't got libc
*/

VIOLET_FORCEINLINE static void 
    memset_as_u8(void* fp_Destination, uint8_t fp_Value, size_t fp_Size)
{
    uint8_t* f_DestinationAsBytes = (uint8_t*)fp_Destination;

    for (size_t lv_Index = 0; lv_Index < fp_Size; lv_Index++)
    {
        f_DestinationAsBytes[lv_Index] = fp_Value;
    }
}

#endif /*VIOLET_KERNEL_VKLIB_MEMSET_HG*/