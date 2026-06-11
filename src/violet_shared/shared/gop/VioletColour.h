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

#define VIOLET_COLOUR(fp_R, fp_G, fp_B) ((VioletGop_Colour){ .Blue = (fp_B), .Green = (fp_G), .Red = (fp_R), .Garbage = 0 })

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

typedef struct {    //little endian ig
    uint8_t Blue;
    uint8_t Green;
    uint8_t Red;
    uint8_t Garbage; //mainly just padding, idk if an alpha channel can be used from uefi; memory aligned accesses babbby
} VioletGop_Colour;


#endif /*VIOLET_SHARED_VIOLET_COLOUR_HG*/