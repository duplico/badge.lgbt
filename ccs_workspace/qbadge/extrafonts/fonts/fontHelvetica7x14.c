//*****************************************************************************
//
// Copyright 1984-1989, 1994 Adobe Systems Incorporated.
// Copyright 1988, 1994 Digital Equipment Corporation.
// 
// Adobe is a trademark of Adobe Systems Incorporated which may be
// registered in certain jurisdictions.
// Permission to use these trademarks is hereby granted only in
// association with the images described in this file.
// 
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose and without fee is hereby
// granted, provided that the above copyright notices appear in all
// copies and that both those copyright notices and this permission
// notice appear in supporting documentation, and that the names of
// Adobe Systems and Digital Equipment Corporation not be used in
// advertising or publicity pertaining to distribution of the software
// without specific, written prior permission.  Adobe Systems and
// Digital Equipment Corporation make no representations about the
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
//     Style: Helvetica
//     Size: 7x14
//     Bold: no
//     Italic: no
//     Memory usage: 1776 bytes
//
//*****************************************************************************

static const uint8_t g_pui8helvetica7x14Data[1576] =
{
      5,   9,   0,  14,  80,  14,   9, 145, 129, 129, 129, 129,
    129, 129, 240,  33, 240, 240,  80,  11,   9, 145,  17,  97,
     17,  97,  17,   0,  10, 112,  20,   9, 240,  81,  17,  97,
     17,  70,  81,  17,  70,  65,  17,  97,  17,  97,  17, 240,
    240,  32,  22,   9, 177, 115,  81,  17,  17,  65,  17, 115,
    113,  17,  65,  17,  17,  65,  17,  17,  83, 113, 240, 144,
     25,   9, 162,  49,  33,  33,  17,  49,  33,  17,  66,  17,
    129, 113,  34,  65,  17,  33,  49,  17,  33,  33,  50, 240,
    208,  21,   9, 178,  97,  33,  81,  33,  98,  97,  17,  81,
     49,  17,  33,  65,  49,  50,  67,  33, 240, 224,   8,   9,
    145, 129, 129,   0,  11,  16,  15,   9, 177, 113, 129, 113,
    129, 129, 129, 129, 129, 145, 129, 145,  96,  15,   9, 145,
    145, 129, 145, 129, 129, 129, 129, 129, 113, 129, 113, 128,
     10,   9, 145,  17, 113, 113,  17,   0,  10, 112,  12,   9,
    240, 240, 129, 129, 101,  97, 129, 240, 240, 192,   9,   9,
      0,  10,  33, 129, 113, 240,  32,   8,   9,   0,   6, 100,
      0,   7,  48,   8,   9,   0,  10,  17, 240, 240,  80,  14,
      9, 193, 129, 113, 129, 113, 129, 129, 113, 129, 240, 240,
     80,  21,   9, 163,  81,  49,  65,  49,  65,  49,  65,  49,
     65,  49,  65,  49,  65,  49,  83, 240, 240,  32,  14,   9,
    177,  99, 129, 129, 129, 129, 129, 129, 129, 240, 240,  48,
     15,   9, 163,  81,  49, 129, 113, 113, 113, 113, 129, 133,
    240, 240,  16,  17,   9, 163,  81,  49, 129,  98, 145, 129,
     65,  49,  65,  49,  83, 240, 240,  32,  18,   9, 209, 114,
     97,  17,  97,  17,  81,  33,  65,  49,  70, 113, 129, 240,
    240,  16,  16,   9, 149,  65, 129, 132, 145, 129,  65,  49,
     65,  49,  83, 240, 240,  32,  20,   9, 163,  81,  49,  65,
    129,  18,  82,  33,  65,  49,  65,  49,  65,  49,  83, 240,
    240,  32,  14,   9, 149, 129, 113, 129, 113, 129, 129, 113,
    129, 240, 240,  64,  20,   9, 163,  81,  49,  65,  49,  83,
     81,  49,  65,  49,  65,  49,  65,  49,  83, 240, 240,  32,
     18,   9, 163,  81,  49,  65,  49,  65,  49,  84, 129, 129,
     65,  49,  83, 240, 240,  32,  11,   9, 240, 240,  97, 240,
    240, 225, 240, 240,  80,  12,   9, 240, 240, 113, 240, 240,
    225, 129, 113, 240,  32,  12,   9, 240, 240, 162,  82,  82,
    146, 146, 240, 240, 144,   9,   9, 240, 240, 245, 213,   0,
      6,  16,  12,   9, 240, 240,  98, 146, 146,  82,  82, 240,
    240, 208,  16,   9, 163,  81,  49,  65,  49, 113, 129, 113,
    129, 240,  33, 240, 240,  48,  27,   9, 197,  34,  81,  17,
     34,  17,  17,  33,  33,  33,  17,  49,  33,  17,  49,  33,
     17,  34,  18,  34,  18,  33, 149, 240,  80,  20,   9, 193,
    113,  17,  97,  17,  81,  49,  65,  49,  69,  49,  81,  33,
     81,  33,  81, 240, 224,  20,   9, 149,  65,  65,  49,  65,
     49,  65,  53,  65,  65,  49,  65,  49,  65,  53, 240, 240,
     16,  15,   9, 180,  65,  65,  33, 129, 129, 129, 129, 145,
     65,  68, 240, 240,  21,   9, 149,  65,  65,  49,  81,  33,
     81,  33,  81,  33,  81,  33,  81,  33,  65,  53, 240, 240,
     16,  13,   9, 150,  49, 129, 129, 134,  49, 129, 129, 134,
    240, 240,  14,   9, 150,  49, 129, 129, 133,  65, 129, 129,
    129, 240, 240,  80,  19,   9, 180,  65,  65,  33, 129, 129,
     51,  33,  81,  33,  81,  49,  50,  67,  17, 240, 224,  21,
      9, 145,  81,  33,  81,  33,  81,  33,  81,  39,  33,  81,
     33,  81,  33,  81,  33,  81, 240, 224,  14,   9, 145, 129,
    129, 129, 129, 129, 129, 129, 129, 240, 240,  80,  16,   9,
    209, 129, 129, 129, 129, 129,  65,  49,  65,  49,  83, 240,
    240,  32,  21,   9, 145,  65,  49,  49,  65,  33,  81,  17,
     99,  97,  33,  81,  49,  65,  65,  49,  81, 240, 224,  14,
      9, 145, 129, 129, 129, 129, 129, 129, 129, 133, 240, 240,
     16,  24,   9, 145, 115,  84,  83,  17,  49,  18,  17,  49,
     18,  33,  17,  34,  33,  17,  34,  49,  50,  49,  49, 240,
    192,  27,   9, 145,  81,  34,  65,  33,  17,  49,  33,  17,
     49,  33,  33,  33,  33,  49,  17,  33,  49,  17,  33,  66,
     33,  81, 240, 224,  20,   9, 180,  65,  65,  33,  97,  17,
     97,  17,  97,  17,  97,  17,  97,  33,  65,  68, 240, 240,
     17,   9, 149,  65,  65,  49,  65,  49,  65,  53,  65, 129,
    129, 129, 240, 240,  80,  23,   9, 180,  65,  65,  33,  97,
     17,  97,  17,  97,  17,  49,  33,  17,  65,  17,  33,  65,
     68,  17, 240, 208,  20,   9, 149,  65,  65,  49,  65,  49,
     65,  53,  65,  49,  65,  65,  49,  65,  49,  65, 240, 240,
     17,   9, 164,  65,  65,  49, 146, 146, 145,  49,  65,  49,
     65,  68, 240, 240,  16,  14,   9, 151,  81, 129, 129, 129,
    129, 129, 129, 129, 240, 240,  32,  22,   9, 145,  65,  49,
     65,  49,  65,  49,  65,  49,  65,  49,  65,  49,  65,  49,
     65,  68, 240, 240,  16,  21,   9, 145,  81,  33,  81,  49,
     49,  65,  49,  65,  49,  81,  17,  97,  17, 113, 129, 240,
    240,  32,  28,   9, 145,  49,  50,  49,  50,  49,  49,  17,
     33,  33,  33,  17,  17,  17,  33,  17,  17,  17,  49,  49,
     65,  49,  65,  49, 240, 224,  21,   9, 145,  81,  49,  49,
     65,  49,  81,  17, 113, 113,  17,  81,  49,  65,  49,  49,
     81, 240, 224,  19,   9, 145,  81,  33,  81,  49,  49,  65,
     49,  81,  17, 113, 129, 129, 129, 240, 240,  32,  13,   9,
    151, 129, 113, 113, 113, 113, 113, 113, 135, 240, 224,  15,
      9, 146, 113, 129, 129, 129, 129, 129, 129, 129, 129, 129,
    130, 112,  14,   9, 145, 129, 145, 129, 145, 129, 129, 145,
    129, 240, 240,  32,  15,   9, 146, 129, 129, 129, 129, 129,
    129, 129, 129, 129, 129, 114, 112,  11,   9, 240,  81, 113,
     17,  81,  49,   0,   9,  64,   6,   9,   0,  12,  55, 176,
      7,   9,   1, 145,   0,  13,  32,  16,   9, 240, 211,  81,
     49, 129,  84,  65,  49,  65,  49,  83,  17, 240, 240,  21,
      9, 145, 129, 129,  18,  82,  33,  65,  49,  65,  49,  65,
     49,  66,  33,  65,  18, 240, 240,  32,  15,   9, 240, 211,
     81,  49,  65, 129, 129, 129,  49,  83, 240, 240,  32,  21,
      9, 209, 129,  82,  17,  65,  34,  65,  49,  65,  49,  65,
     49,  65,  34,  82,  17, 240, 240,  16,  16,   9, 240, 211,
     81,  49,  65,  49,  69,  65, 129,  49,  83, 240, 240,  32,
     14,   9, 178,  97, 115, 113, 129, 129, 129, 129, 129, 240,
    240,  64,  22,   9, 240, 210,  17,  65,  34,  65,  49,  65,
     49,  65,  49,  65,  34,  82,  17, 129,  65,  49,  83,  80,
     21,   9, 145, 129, 129,  18,  82,  33,  65,  49,  65,  49,
     65,  49,  65,  49,  65,  49, 240, 240,  16,  14,   9, 145,
    240,  33, 129, 129, 129, 129, 129, 129, 240, 240,  80,  15,
      9, 161, 240,  33, 129, 129, 129, 129, 129, 129, 129, 129,
    113, 128,  19,   9, 145, 129, 129,  33,  81,  17,  98, 114,
    113,  17,  97,  33,  81,  49, 240, 240,  16,  14,   9, 145,
    129, 129, 129, 129, 129, 129, 129, 129, 240, 240,  80,  26,
      9, 240, 193,  17,  33,  50,  18,  17,  33,  33,  33,  33,
     33,  33,  33,  33,  33,  33,  33,  33,  33,  33,  33, 240,
    224,  20,   9, 240, 193,  18,  82,  33,  65,  49,  65,  49,
     65,  49,  65,  49,  65,  49, 240, 240,  16,  18,   9, 240,
    211,  81,  49,  65,  49,  65,  49,  65,  49,  65,  49,  83,
    240, 240,  32,  21,   9, 240, 193,  18,  82,  33,  65,  49,
     65,  49,  65,  49,  66,  33,  65,  18,  81, 129, 129, 128,
     21,   9, 240, 210,  17,  65,  34,  65,  49,  65,  49,  65,
     49,  65,  34,  82,  17, 129, 129, 129,  64,  14,   9, 240,
    193,  17,  98, 113, 129, 129, 129, 129, 240, 240,  80,  15,
      9, 240, 210,  97,  33,  81, 146, 145,  81,  33,  98, 240,
    240,  48,  14,   9, 161, 129, 115, 113, 129, 129, 129, 129,
    130, 240, 240,  48,  20,   9, 240, 193,  49,  65,  49,  65,
     49,  65,  49,  65,  49,  65,  34,  82,  17, 240, 240,  16,
     18,   9, 240, 193,  49,  65,  49,  65,  49,  81,  17,  97,
     17, 113, 129, 240, 240,  48,  24,   9, 240, 193,  49,  50,
     49,  49,  17,  33,  33,  33,  33,  33,  33,  17,  17,  17,
     49,  49,  65,  49, 240, 224,  17,   9, 240, 193,  65,  65,
     33,  98, 114,  97,  33,  65,  65,  49,  65, 240, 240,  20,
      9, 240, 193,  49,  65,  49,  65,  49,  65,  33,  97,  17,
     97,  17, 113, 129, 113, 113, 128,  13,   9, 240, 196, 129,
    113, 113, 129, 113, 132, 240, 240,  32,  15,   9, 178,  97,
    129, 129, 129, 113, 145, 129, 129, 129, 129, 146,  80,  15,
      9, 145, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129,
    129, 128,  15,   9, 146, 145, 129, 129, 129, 145, 113, 129,
    129, 129, 129,  98, 112,  11,   9,   0,   5,  98,  33,  49,
     34,   0,   7,  32,
};

const tFont g_sFonthelvetica7x14 =
{
    //
    // The format of the font.
    //
    FONT_FMT_PIXEL_RLE,

    //
    // The maximum width of the font.
    //
    9,

    //
    // The height of the font.
    //
    13,

    //
    // The baseline of the font.
    //
    10,

    //
    // The offset to each character in the font.
    //
    {
           0,    5,   19,   30,   50,   72,   97,  118,
         126,  141,  156,  166,  178,  187,  195,  203,
         217,  238,  252,  267,  284,  302,  318,  338,
         352,  372,  390,  401,  413,  425,  434,  446,
         462,  489,  509,  529,  544,  565,  578,  592,
         611,  632,  646,  662,  683,  697,  721,  748,
         768,  785,  808,  828,  845,  859,  881,  902,
         930,  951,  970,  983,  998, 1012, 1027, 1038,
        1044, 1051, 1067, 1088, 1103, 1124, 1140, 1154,
        1176, 1197, 1211, 1226, 1245, 1259, 1285, 1305,
        1323, 1344, 1365, 1379, 1394, 1408, 1428, 1446,
        1470, 1487, 1507, 1520, 1535, 1550, 1565,
    },

    //
    // A pointer to the actual font data
    //
    g_pui8helvetica7x14Data
};
