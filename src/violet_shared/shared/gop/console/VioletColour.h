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
#ifndef VIOLET_SHARED_VIOLET_COLOUR_HG
#define VIOLET_SHARED_VIOLET_COLOUR_HG

#include <stdint.h>

typedef struct {    //little endian ig
    uint8_t Blue;
    uint8_t Green;
    uint8_t Red;
    uint8_t Garbage; //mainly just padding, idk if an alpha channel can be used from uefi; memory aligned accesses babbby
} VioletColour;


#endif /*VIOLET_SHARED_VIOLET_COLOUR_HG*/