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
//     Style: Times
//     Size: 6x12
//     Bold: yes
//     Italic: no
//     Memory usage: 1532 bytes
//
//*****************************************************************************

static const uint8_t g_pui8times6x12bData[1329] =
{
      5,  10,   0,  13,  96,  12,  10, 162, 130, 130, 130, 240,
     50, 130, 240, 240, 128,  11,  10, 161,  17, 113,  17, 113,
     17,   0,   9,  80,  17,  10, 177,  17, 113,  17, 101,  97,
     17, 101,  97,  17, 113,  17, 240, 240,  96,  17,  10,  33,
    131,  97,  17,  17,  83, 131, 131,  81,  17,  17,  84, 129,
    240, 192,  19,  10, 179,  18,  50,  19,  67,  17, 129, 129,
     19,  65,  18,  17,  49,  35, 240, 240,  64,  18,  10, 194,
    114,  17,  98,  17,  99,  18,  50,  19,  66,  34,  83,  18,
    240, 240,  48,   8,  10, 161, 145, 145,   0,   9, 112,  13,
     10, 193, 130, 129, 130, 130, 130, 130, 145, 146, 145, 112,
     13,  10, 161, 146, 145, 146, 130, 130, 130, 129, 130, 129,
    144,  10,  10, 193, 117,  99, 101, 113,   0,   7,  16,  12,
     10, 240, 240,  33, 145, 117, 113, 145, 240, 240, 112,   9,
     10,   0,   7,  66, 130, 145, 240, 208,   8,  10,   0,   6,
     35,   0,   7,  16,   9,  10,   0,   7,  66, 130, 240, 240,
    128,  12,  10, 193, 145, 130, 129, 130, 129, 145, 240, 240,
    144,  17,  10, 178, 114,  17,  98,  17,  98,  17,  98,  17,
     98,  17, 114, 240, 240, 112,  12,  10, 178, 115, 130, 130,
    130, 130, 116, 240, 240,  96,  12,  10, 178, 116, 130, 129,
    129, 132, 100, 240, 240,  96,  14,  10, 178, 113,  18, 130,
    114, 146,  97,  18,  99, 240, 240, 112,  14,  10, 209, 130,
    113,  17,  97,  33, 101, 114, 130, 240, 240,  96,  13,  10,
    163, 115, 113, 147, 146,  97,  18,  99, 240, 240, 112,  14,
     10, 194, 114, 114, 132,  98,  17,  98,  17, 114, 240, 240,
    112,  13,  10, 180,  84,  97,  33, 130, 129, 130, 130, 240,
    240, 112,  16,  10, 178, 114,  17,  98,  17, 114, 113,  18,
     97,  18, 114, 240, 240, 112,  15,  10, 178, 113,  18,  97,
     18,  97,  18, 115, 129, 114, 240, 240, 128,  11,  10, 240,
    242, 130, 240,  50, 130, 240, 240, 128,  11,  10, 240, 242,
    130, 240,  50, 130, 145, 240, 208,  12,  10, 240, 240,  50,
     99,  98, 147, 146, 240, 240,  80,   9,  10, 240, 240, 165,
    245, 240, 240, 240,  11,  10, 240, 242, 147, 146,  99,  98,
    240, 240, 128,  14,  10, 178, 113,  18,  97,  18, 129, 240,
     50, 130, 240, 240, 112,  23,  10, 229,  50,  66,  18,  18,
     17,  19,  18,  17,  35,  17,  33,  35,  17,  33,  35,  34,
     18,  34, 149, 208,  15,  10, 209, 146, 115, 113,  18,  85,
     81,  35,  51,  20, 240, 240,  32,  16,  10, 165,  98,  18,
     82,  18,  84,  98,  18,  82,  18,  69, 240, 240,  80,  14,
     10, 181,  66,  34,  66, 130, 130, 131,  18,  84, 240, 240,
     80,  17,  10, 166,  82,  34,  66,  34,  66,  34,  66,  34,
     66,  34,  54, 240, 240,  64,  14,  10, 166,  82,  33,  82,
    131, 114, 130,  33,  70, 240, 240,  64,  15,  10, 166,  82,
     33,  82,  17, 100,  98,  17,  98, 116, 240, 240,  96,  16,
     10, 181,  66,  34,  66, 130,  19,  66,  34,  66,  34,  84,
    240, 240,  80,  18,  10, 163,  35,  50,  34,  66,  34,  70,
     66,  34,  66,  34,  51,  35, 240, 240,  32,  12,  10, 164,
    114, 130, 130, 130, 130, 116, 240, 240,  96,  12,  10, 164,
    114, 130, 130, 130, 130, 130, 114, 240, 208,  18,  10, 163,
     19,  66,  33,  82,  17, 100,  98,  18,  82,  19,  51,  35,
    240, 240,  32,  14,  10, 164, 114, 130, 130, 130,  33,  82,
     18,  70, 240, 240,  64,  22,  10, 163,  51,  35,  34,  51,
     19,  49,  18,  18,  49,  18,  18,  49,  33,  18,  35,  51,
    240, 240,  16,  20,  10, 163,  19,  66,  33,  83,  17,  81,
     17,  17,  81,  19,  81,  34,  67,  33, 240, 240,  64,  17,
     10, 180,  82,  34,  66,  34,  66,  34,  66,  34,  66,  34,
     84, 240, 240,  80,  14,  10, 165,  98,  18,  82,  18,  84,
     98, 130, 116, 240, 240,  96,  17,  10, 180,  82,  34,  66,
     34,  66,  34,  66,  34,  66,  34,  84, 114, 147, 224,  17,
     10, 165,  98,  18,  82,  18,  84,  98,  17,  98,  18,  67,
     19, 240, 240,  48,  14,  10, 180,  82,  33,  83, 131, 131,
     81,  34,  84, 240, 240,  96,  14,  10, 166,  65,  18,  17,
     98, 130, 130, 130, 116, 240, 240,  80,  18,  10, 164,  18,
     66,  33,  82,  33,  82,  33,  82,  33,  82,  33,  99, 240,
    240,  80,  16,  10, 164,  34,  50,  49,  67,  17,  98,  17,
     99, 130, 129, 240, 240,  96,  23,  10, 163,  19,  18,  18,
     18,  33,  34,  18,  18,  34,  18,  17,  66,  19,  66,  18,
     97,  33, 240, 240,  48,  17,  10, 163,  34,  66,  33,  99,
    115, 113,  18,  81,  50,  50,  35, 240, 240,  48,  15,  10,
    164,  19,  50,  49,  82,  17,  99, 130, 130, 116, 240, 240,
     64,  16,  10, 166,  66,  34,  65,  34, 114, 114,  33,  66,
     34,  70, 240, 240,  64,  13,  10, 163, 114, 130, 130, 130,
    130, 130, 130, 130, 131, 112,  12,  10, 161, 145, 146, 145,
    146, 145, 145, 240, 240, 112,  13,  10, 163, 130, 130, 130,
    130, 130, 130, 130, 130, 115, 112,  11,  10, 193, 131,  98,
     18,  81,  49,   0,   8,  16,   6,  10,   0,  12,  69,  80,
      7,  10,   1, 161,   0,  12,  32,  14,  10, 240, 240,  18,
    113,  18, 115,  97,  33, 101, 240, 240,  80,  15,  10, 162,
    130, 131, 114,  17,  98,  17,  98,  17,  99, 240, 240, 112,
     14,  10, 240, 240,  19,  98,  17,  98, 130,  17, 115, 240,
    240,  96,  16,  10, 195, 130, 100,  82,  18,  82,  18,  82,
     18,  98,  18, 240, 240,  64,  13,  10, 240, 240,  19,  98,
     17, 100,  98, 147, 240, 240,  96,  12,  10, 178, 114, 131,
    114, 130, 130, 131, 240, 240, 112,  16,  10, 240, 240,  20,
     82,  17,  98,  17, 115,  98, 132,  97,  33, 100,  96,  16,
     10, 162, 130, 132,  98,  17,  98,  17,  98,  17,  98,  18,
    240, 240,  80,  13,  10,   2, 130, 240,  50, 130, 130, 130,
    130, 240, 240, 128,  14,  10,  18, 130, 240,  50, 130, 130,
    130, 130, 130, 114, 240,  48,  16,  10, 162, 130, 130,  18,
     82,  17,  99, 114,  17,  98,  18, 240, 240,  80,  12,  10,
    162, 130, 130, 130, 130, 130, 130, 240, 240, 128,  19,  10,
    240, 241,  17,  18,  71,  50,  17,  18,  50,  17,  18,  50,
     17,  19, 240, 240,  32,  16,  10, 240, 241,  18,  98,  18,
     82,  18,  82,  18,  82,  19, 240, 240,  64,  15,  10, 240,
    240,  19,  98,  18,  82,  18,  82,  18,  99, 240, 240,  96,
     15,  10, 240, 243, 114,  17,  98,  17,  98,  17,  99, 114,
    131, 240,  32,  15,  10, 240, 240,  19,  98,  17,  98,  17,
     98,  17, 115, 130, 131, 240,  12,  10, 240, 242,  17, 100,
     98, 130, 130, 240, 240, 128,  12,  10, 240, 240,  18, 114,
    131, 130, 115, 240, 240, 112,  12,  10, 177, 130, 131, 114,
    130, 130, 146, 240, 240, 112,  15,  10, 240, 242,  18,  82,
     17,  98,  17,  98,  17, 115, 240, 240,  96,  13,  10, 240,
    242,  18,  82,  17, 115, 114, 145, 240, 240, 112,  17,  10,
    240, 241,  17,  18,  65,  17,  17,  81,  19,  84, 113,  17,
    240, 240,  96,  13,  10, 240, 242,  17, 100, 114, 116,  97,
     18, 240, 240,  96,  15,  10, 240, 242,  18,  82,  18,  99,
    115, 129, 113,  17, 115, 240,  32,  13,  10, 240, 244,  97,
     18, 114, 114,  17, 100, 240, 240,  96,  13,  10, 194, 114,
    130, 129, 130, 130, 145, 146, 130, 146,  96,  13,  10, 161,
    145, 145, 145, 145, 145, 145, 145, 145, 240,  64,  13,  10,
    162, 146, 130, 145, 146, 130, 129, 130, 130, 114, 128,  10,
     10, 240, 240, 178,  17,  81,  18,   0,   7,
};

const tFont g_sFonttimes6x12b =
{
    //
    // The format of the font.
    //
    FONT_FMT_PIXEL_RLE,

    //
    // The maximum width of the font.
    //
    10,

    //
    // The height of the font.
    //
    11,

    //
    // The baseline of the font.
    //
    8,

    //
    // The offset to each character in the font.
    //
    {
           0,    5,   17,   28,   45,   62,   81,   99,
         107,  120,  133,  143,  155,  164,  172,  181,
         193,  210,  222,  234,  248,  262,  275,  289,
         302,  318,  333,  344,  355,  367,  376,  387,
         401,  424,  439,  455,  469,  486,  500,  515,
         531,  549,  561,  573,  591,  605,  627,  647,
         664,  678,  695,  712,  726,  740,  758,  774,
         797,  814,  829,  845,  858,  870,  883,  894,
         900,  907,  921,  936,  950,  966,  979,  991,
        1007, 1023, 1036, 1050, 1066, 1078, 1097, 1113,
        1128, 1143, 1158, 1170, 1182, 1194, 1209, 1222,
        1239, 1252, 1267, 1280, 1293, 1306, 1319,
    },

    //
    // A pointer to the actual font data
    //
    g_pui8times6x12bData
};