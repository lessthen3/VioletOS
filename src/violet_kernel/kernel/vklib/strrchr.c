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
#include <stddef.h>

void* 
    strrchr(const char* fp_Str, int fp_Char)
{
    const char* f_Last = NULL;
    
    // Scan the entire string including the null terminator
    do {
        if (*fp_Str == (char)fp_Char) 
        {
            f_Last = fp_Str;
        }
    } while (*fp_Str++);
    
    return (void*)f_Last;
}