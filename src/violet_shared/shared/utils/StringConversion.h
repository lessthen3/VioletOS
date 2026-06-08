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
#ifndef VIOLET_SHARED_STRING_CONVERSION_HG
#define VIOLET_SHARED_STRING_CONVERSION_HG

///CSTD
#include <stddef.h>

///VioletShared
#include "shared/GeneralMacros.h"

[[nodiscard]] VIOLET_FORCEINLINE const char* 
    uintn_to_str(size_t value) 
{
    // A 64-bit integer requires at most 20 digits + 1 null terminator
    static char buffer[21]; 
    char* ptr = &buffer[20];
    *ptr = '\0'; // Null terminate the string

    // Special case for 0
    if (value == 0) 
    {
        *--ptr = '0';
        return ptr;
    }

    // Process digits from right to left
    while (value > 0) 
    {
        *--ptr = (char)('0' + (value % 10));
        value /= 10;
    }

    return ptr;
}

#endif /*VIOLET_SHARED_STRING_CONVERSION_HG*/