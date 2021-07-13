//*****************************************************************************
//
// Copyright 1989 Dale Schumacher, dal@syntel.mn.org
//                399 Beacon Ave.
//                St. Paul, MN  55104-3527
// 
// Permission to use, copy, modify, and distribute this software and
// its documentation for any purpose and without fee is hereby
// granted, provided that the above copyright notice appear in all
// copies and that both that copyright notice and this permission
// notice appear in supporting documentation, and that the name of
// Dale Schumacher not be used in advertising or publicity pertaining to
// distribution of the software without specific, written prior
// permission.  Dale Schumacher makes no representations about the
// suitability of this software for any purpose.  It is provided "as
// is" without express or implied warranty.
//*****************************************************************************

//*****************************************************************************
//
// This file is generated by ftrasterize; DO NOT EDIT BY HAND!
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "grlib/grlib.h"

//*****************************************************************************
//
// Details of this font:
//     Characters: 32 to 126 inclusive
//     Style: Clean
//     Size: 7x14
//     Bold: no
//     Italic: no
//     Memory usage: 1556 bytes
//
//*****************************************************************************

static const uint8_t g_pui8clean7x14Data[1355] =
{
      5,   7,   0,  12,  32,  12,   3,  49,  33,  33,  33,  33,
     33,  33,  81,  33, 176,  11,   5, 161,  17,  33,  17,  33,
     17,   0,   5, 112,  23,   6, 113,  33,  33,  33,  33,  33,
     22,  17,  33,  33,  33,  33,  33,  22,  17,  33,  33,  33,
     33,  33, 208,  19,   6, 225,  67,  33,  17,  17,  17,  17,
     66,  82,  65,  17,  17,  17,  17,  35,  65, 240,  14,   6,
    240,  50,  66,  33,  65,  65,  65,  65,  34,  66, 240, 160,
     17,   6, 211,  33,  81,  97,  82,  49,  33,  18,  49,  17,
     49,  35,  17, 240,  48,   8,   5, 178,  49,  49,   0,   6,
     16,  14,   5, 113,  49,  65,  49,  65,  65,  65,  65,  81,
     65,  81, 192,  14,   5,  81,  81,  65,  81,  65,  65,  65,
     65,  49,  65,  49, 224,  16,   7, 240, 145,  97,  50,  17,
     18,  35,  81,  81,  17,  49,  49, 240, 224,  11,   6, 240,
    177,  81,  53,  49,  81, 240, 240,  48,   9,   6,   0,   6,
    114,  66,  65,  65, 176,   8,   6, 240, 240, 102, 240, 240,
    192,   8,   5, 240, 240, 242,  50, 240,  48,  16,   6,  81,
     81,  65,  81,  65,  81,  65,  81,  65,  81,  65,  81, 240,
     32,  22,   6, 115,  33,  49,  17,  49,  17,  49,  17,  49,
     17,  49,  17,  49,  17,  49,  17,  49,  35, 240,  80,  14,
      5,  97,  50,  65,  65,  65,  65,  65,  65,  65,  65, 240,
     48,  15,   6, 115,  33,  49,  81,  81,  65,  65,  65,  65,
     81,  85, 240,  64,  16,   6, 115,  33,  49,  81,  81,  50,
     97,  81,  81,  17,  49,  35, 240,  80,  17,   6, 145,  81,
     66,  66,  49,  17,  49,  17,  33,  33,  37,  65,  67, 240,
     64,  15,   6, 101,  17,  81,  81,  84,  97,  81,  81,  17,
     49,  35, 240,  80,  18,   6, 130,  49,  65,  81,  84,  33,
     49,  17,  49,  17,  49,  17,  49,  35, 240,  80,  15,   6,
    101,  17,  49,  81,  81,  65,  81,  81,  65,  81,  81, 240,
     96,  21,   6, 115,  33,  49,  17,  49,  17,  49,  35,  33,
     49,  17,  49,  17,  49,  17,  49,  35, 240,  80,  18,   6,
    115,  33,  49,  17,  49,  17,  49,  17,  49,  36,  81,  81,
     65,  50, 240,  96,  10,   5, 240,  82,  50, 240,  50,  50,
    240,  48,  11,   6, 240, 162,  66, 240, 114,  66,  65,  65,
    176,  10,   6, 240, 210,  34,  34,  98,  98, 240, 240,   7,
      6, 240, 246, 198, 240, 240,  11,   6, 240, 146,  98,  98,
     34,  34, 240, 240,  64,  14,   6, 116,  17,  65,  81,  81,
     65,  65,  81, 177,  81, 240,  80,  21,   7, 240, 131,  49,
     49,  17,  33,  34,  17,  17,  18,  17,  17,  18,  33,  17,
     33, 116, 240, 112,  19,   7, 240,  33,  97,  83,  65,  17,
     65,  17,  53,  33,  49,  33,  49,  18,  50, 240,  96,  13,
      6, 197,  17,  66,  66,  70,  17,  66,  66,  70, 240,  64,
     14,   6, 227,  33,  50,  81,  81,  81,  81,  97,  49,  35,
    240,  64,  15,   6, 196,  33,  49,  17,  66,  66,  66,  66,
     66,  49,  20, 240,  80,  12,   6, 199,  81,  81,  84,  33,
     81,  81,  86, 240,  48,  12,   6, 199,  81,  81,  84,  33,
     81,  81,  81, 240, 128,  15,   6, 227,  33,  50,  81,  81,
     81,  36,  65,  17,  49,  36, 240,  48,  13,   6, 193,  66,
     66,  66,  72,  66,  66,  66,  65, 240,  48,  13,   6, 197,
     49,  81,  81,  81,  81,  81,  81,  53, 240,  64,  13,   6,
    243,  81,  81,  81,  81,  82,  66,  65,  20, 240,  64,  20,
      6, 193,  66,  49,  17,  33,  33,  17,  50,  65,  17,  49,
     33,  33,  49,  17,  65, 240,  48,  13,   6, 193,  81,  81,
     81,  81,  81,  81,  81,  86, 240,  48,  16,   6, 193,  66,
     67,  36,  35,  18,  18,  18,  18,  66,  66,  65, 240,  48,
     18,   6, 194,  51,  50,  17,  34,  17,  34,  33,  18,  33,
     18,  51,  51,  65, 240,  48,  16,   6, 226,  49,  33,  17,
     66,  66,  66,  66,  65,  17,  33,  50, 240,  80,  13,   6,
    197,  17,  66,  66,  70,  17,  81,  81,  81, 240, 128,  19,
      6, 226,  49,  33,  17,  66,  66,  66,  66,  65,  17,  33,
     50,  49,  17,  17,  65, 112,  17,   6, 197,  17,  66,  66,
     70,  17,  17,  49,  33,  33,  49,  17,  65, 240,  48,  13,
      6, 212,  17,  66,  81, 100,  97,  82,  65,  20, 240,  64,
     13,   7, 231,  49,  97,  97,  97,  97,  97,  97,  97, 240,
    144,  14,   6, 193,  66,  66,  66,  66,  66,  66,  66,  65,
     20, 240,  64,  19,   7, 226,  50,  17,  49,  33,  49,  49,
     17,  65,  17,  65,  17,  81,  97,  97, 240, 144,  16,   6,
    193,  66,  66,  66,  18,  18,  18,  19,  36,  35,  66,  65,
    240,  48,  21,   6, 193,  49,  17,  49,  33,  17,  49,  17,
     65,  65,  17,  49,  17,  33,  49,  17,  49, 240,  64,  17,
      6, 193,  49,  17,  49,  33,  17,  49,  17,  65,  81,  81,
     81,  81, 240,  96,  13,   6, 197,  81,  65,  81,  65,  65,
     81,  65,  85, 240,  64,  14,   5,  83,  33,  65,  65,  65,
     65,  65,  65,  65,  65,  67, 192,  15,   6,   1,  81,  97,
     81,  97,  81,  97,  81,  97,  81,  97,  81, 192,  14,   5,
     83,  65,  65,  65,  65,  65,  65,  65,  65,  65,  35, 192,
     10,   6, 129,  65,  17,  33,  49,   0,   7,  80,   6,   6,
      0,   9,   6,  96,   8,   5, 162,  65,  81,   0,   5, 112,
     13,   6, 240, 166,  66,  66,  66,  66,  50,  19,  17, 240,
     48,  14,   6,  97,  81,  81,  85,  17,  66,  66,  66,  66,
     70, 240,  64,  12,   6, 240, 164,  17,  81,  81,  81,  81,
    100, 240,  64,  14,   6, 177,  81,  81,  22,  66,  66,  66,
     66,  65,  21, 240,  48,  11,   6, 240, 164,  17,  66,  72,
     81, 100, 240,  64,  14,   6, 132,  17,  81,  68,  49,  81,
     81,  81,  81,  81, 240, 112,  13,   6, 240, 166,  66,  66,
     66,  66,  65,  21,  81,  81,  20,  15,   6,  97,  81,  81,
     85,  17,  66,  66,  66,  66,  66,  65, 240,  48,  13,   6,
    129,  81, 147,  81,  81,  81,  81,  81,  53, 240,  64,  15,
      6, 161,  81, 132,  81,  81,  81,  81,  81,  81,  81,  81,
     20,  32,  19,   6,  97,  81,  81,  81,  66,  49,  17,  33,
     35,  49,  33,  33,  49,  17,  65, 240,  48,  14,   6,  99,
     81,  81,  81,  81,  81,  81,  81,  81,  53, 240,  64,  23,
      6, 240, 146,  17,  33,  17,  17,  17,  17,  17,  17,  17,
     17,  17,  17,  17,  17,  49,  17,  49, 240,  64,  14,   6,
    240, 145,  19,  18,  50,  66,  66,  66,  66,  65, 240,  48,
     13,   6, 240, 164,  17,  66,  66,  66,  66,  65,  20, 240,
     64,  14,   6, 240, 149,  17,  66,  66,  66,  66,  70,  17,
     81,  81,  80,  13,   6, 240, 166,  66,  66,  66,  66,  65,
     21,  81,  81,  81,  13,   6, 240, 145,  19,  18,  65,  81,
     81,  81,  81, 240, 128,  10,   6, 240, 166,  81, 100,  97,
     86, 240,  64,  14,   6, 129,  81,  81,  54,  33,  81,  81,
     81,  81,  99, 240,  48,  14,   6, 240, 145,  66,  66,  66,
     66,  66,  50,  19,  17, 240,  48,  17,   7, 240, 210,  50,
     17,  49,  33,  49,  49,  17,  65,  17,  81,  97, 240, 144,
     23,   6, 240, 145,  49,  17,  49,  17,  17,  17,  17,  17,
     17,  17,  17,  17,  17,  17,  17,  33,  17, 240,  80,  18,
      6, 240, 145,  49,  17,  49,  33,  17,  65,  65,  17,  33,
     49,  17,  49, 240,  64,  14,   6, 240, 145,  66,  66,  66,
     66,  66,  65,  21,  81,  81,  20,  12,   6, 240, 149,  81,
     65,  65,  65,  65,  85, 240,  64,  14,   5, 113,  49,  65,
     65,  65,  49,  81,  65,  65,  65,  81, 192,  14,   4,  65,
     49,  49,  49,  49,  49,  49,  49,  49,  49,  49, 176,  14,
      5,  81,  81,  65,  65,  65,  81,  49,  65,  65,  65,  49,
    224,  10,   6, 113,  65,  17,  17,  65,   0,   7,  96,
};

const tFont g_sFontclean7x14 =
{
    //
    // The format of the font.
    //
    FONT_FMT_PIXEL_RLE,

    //
    // The maximum width of the font.
    //
    7,

    //
    // The height of the font.
    //
    14,

    //
    // The baseline of the font.
    //
    11,

    //
    // The offset to each character in the font.
    //
    {
           0,    5,   17,   28,   51,   70,   84,  101,
         109,  123,  137,  153,  164,  173,  181,  189,
         205,  227,  241,  256,  272,  289,  304,  322,
         337,  358,  376,  386,  397,  407,  414,  425,
         439,  460,  479,  492,  506,  521,  533,  545,
         560,  573,  586,  599,  619,  632,  648,  666,
         682,  695,  714,  731,  744,  757,  771,  790,
         806,  827,  844,  857,  871,  886,  900,  910,
         916,  924,  937,  951,  963,  977,  988, 1002,
        1015, 1030, 1043, 1058, 1077, 1091, 1114, 1128,
        1141, 1155, 1168, 1181, 1191, 1205, 1219, 1236,
        1259, 1277, 1291, 1303, 1317, 1331, 1345,
    },

    //
    // A pointer to the actual font data
    //
    g_pui8clean7x14Data
};
