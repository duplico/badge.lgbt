/*
 * TI Graphics Library (grlib) layer library for the GDEH029A1 e-paper display.
 *
 * Copyright (c) 2019, George Louthan <george@queercon.org>
 * Copyright (c) 2013, Texas Instruments Incorporated
 *
 * BSD 3-clause.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <ti/grlib/grlib.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

#include <board.h>
#include <queercon_drivers/epd.h>
#include <queercon_drivers/epd_phy.h>

uint8_t epd_upside_down = 1;

#define REVERSE_BYTE(b) (uint8_t)(((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16)

void epd_flip() {
    epd_upside_down = !epd_upside_down;
    uint8_t reverse_temp;

    uint16_t high;
    for (uint16_t low=0; low<sizeof(epd_display_buffer)/2; low++) {
        high = (sizeof(epd_display_buffer) - low) - 1;
        reverse_temp = REVERSE_BYTE(epd_display_buffer[low]);
        epd_display_buffer[low] = REVERSE_BYTE(epd_display_buffer[high]);
        epd_display_buffer[high] = reverse_temp;
    }
}

//*****************************************************************************
//
// All the following functions (below) for the LCD driver are required by grlib
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws a pixel on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the pixel.
//! \param lY is the Y coordinate of the pixel.
//! \param ulValue is the color of the pixel.
//!
//! This function sets the given pixel to a particular color.  The coordinates
//! of the pixel are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
// TemplateDisplayFix
static void epd_grPixelDraw(const Graphics_Display * pvDisplayData,
                            int16_t lX, int16_t lY, uint16_t ulValue) {
    if (lX < 0 || lY < 0) {
        return;
    }
    /* This function already has checked that the pixel is within the extents of
       the LCD screen and the color ulValue has already been translated to the LCD.
     */
    uint16_t mapped_x;
    uint16_t mapped_y;

    if (pvDisplayData->width < pvDisplayData->heigth) {
        // We're dealing with the PORTRAIT context
        mapped_x = MAPPED_X_PORTRAIT(lX, lY);
        mapped_y = MAPPED_Y_PORTRAIT(lX, lY);
    } else if (!epd_upside_down) {
        // Right-side-up LANDSCAPE
        mapped_x = MAPPED_X(lX, lY);
        mapped_y = MAPPED_Y(lX, lY);
    } else {
        // Flipped LANDSCAPE
        mapped_x = MAPPED_X_ROTATED(lX, lY);
        mapped_y = MAPPED_Y_ROTATED(lX, lY);
    }

    // Now, mapped_x and mapped_y are the correct indices, transformed
    //  if necessary, to index our pixel buffer. We can get the needed
    //  byte, with GRAM_BUFFER. And then we need to RIGHT-shift the
    //  X coordinate, as needed.
    uint8_t val = BIT7 >> (mapped_x % 8);

    volatile uint16_t buffer_addr;
    buffer_addr = ((LCD_X_SIZE/8) * mapped_y) + (mapped_x / 8);

    if (ulValue) {
        epd_display_buffer[buffer_addr] |= val;
    } else {
        epd_display_buffer[buffer_addr] &= ~val;
    }
}

//*****************************************************************************
//
//! Draws a horizontal sequence of pixels on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the first pixel.
//! \param lY is the Y coordinate of the first pixel.
//! \param lX0 is sub-pixel offset within the pixel data, which is valid for 1
//! or 4 bit per pixel formats.
//! \param lCount is the number of pixels to draw.
//! \param lBPP is the number of bits per pixel; must be 1, 4, or 8.
//! \param pucData is a pointer to the pixel data.  For 1 and 4 bit per pixel
//! formats, the most significant bit(s) represent the left-most pixel.
//! \param pucPalette is a pointer to the palette used to draw the pixels.
//!
//! This function draws a horizontal sequence of pixels on the screen, using
//! the supplied palette.  For 1 bit per pixel format, the palette contains
//! pre-translated colors; for 4 and 8 bit per pixel formats, the palette
//! contains 24-bit RGB values that must be translated before being written to
//! the display.
//!
//! \return None.
//
//*****************************************************************************
static void epd_grPixelDrawMultiple(const Graphics_Display * pvDisplayData,
                                    int16_t lX, int16_t lY, int16_t lX0,
                                    int16_t lCount, int16_t lBPP,
                                    const uint8_t *pucData,
                                    const uint32_t *pucPalette) {
    uint16_t ulByte;
    // Loop while there are more pixels to draw
    while(lCount > 0)
    {
        // Get the next byte of image data
        ulByte = *pucData++;

        // Loop through the pixels in this byte of image data
        for(; (lX0 < 8) && lCount; lX0++, lCount--)
        {
            volatile uint16_t val;
            val = (ulByte >> (7 - lX0)) & 1;
            epd_grPixelDraw(pvDisplayData, lX, lY, val);
//            // Draw this pixel in the appropriate color
//            if (((uint16_t *)pucPalette)[(ulByte >> (7 - lX0)) & 1]) {
//                epd_grPixelDraw(pvDisplayData, lX, lY, 1);
//            }
            lX++;
        }

        // Start at the beginning of the next byte of image data
        lX0 = 0;
    }
}

//*****************************************************************************
//
//! Draws a horizontal line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX1 is the X coordinate of the start of the line.
//! \param lX2 is the X coordinate of the end of the line.
//! \param lY is the Y coordinate of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a horizontal line on the display.  The coordinates of
//! the line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void epd_grLineDrawH(const Graphics_Display * pvDisplayData,
                            int16_t lX1, int16_t lX2, int16_t lY,
                            uint16_t ulValue) {
    /* Ideally this function shouldn't call pixel draw. It should have it's own
  definition using the built in auto-incrementing of the LCD controller and its
  own calls to SetAddress() and WriteData(). Better yet, SetAddress() and WriteData()
  can be made into macros as well to eliminate function call overhead. */

    do
    {
        epd_grPixelDraw(pvDisplayData, lX1, lY, ulValue);
    }
    while(lX1++ < lX2);
}

//*****************************************************************************
//
//! Draws a vertical line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the line.
//! \param lY1 is the Y coordinate of the start of the line.
//! \param lY2 is the Y coordinate of the end of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a vertical line on the display.  The coordinates of the
//! line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void epd_grLineDrawV(const Graphics_Display * pvDisplayData, int16_t lX,
                            int16_t lY1, int16_t lY2, uint16_t ulValue) {
    do
    {
        epd_grPixelDraw(pvDisplayData, lX, lY1, ulValue);
    }
    while(lY1++ < lY2);
}

//*****************************************************************************
//
//! Fills a rectangle.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param pRect is a pointer to the structure describing the rectangle.
//! \param ulValue is the color of the rectangle.
//!
//! This function fills a rectangle on the display.  The coordinates of the
//! rectangle are assumed to be within the extents of the display, and the
//! rectangle specification is fully inclusive (in other words, both sXMin and
//! sXMax are drawn, along with sYMin and sYMax).
//!
//! \return None.
//
//*****************************************************************************
static void epd_grRectFill(const Graphics_Display * pvDisplayData,
                           const tRectangle *pRect, uint16_t ulValue) {
    int16_t x0 = pRect->sXMin;
    int16_t x1 = pRect->sXMax;
    int16_t y0 = pRect->sYMin;
    int16_t y1 = pRect->sYMax;

    while(y0++ <= y1)
    {
        epd_grLineDrawH(pvDisplayData, x0, x1, y0, ulValue);
    }
}

//*****************************************************************************
//
//! Translates a 24-bit RGB color to a display driver-specific color.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param ulValue is the 24-bit RGB color.  The least-significant byte is the
//! blue channel, the next byte is the green channel, and the third byte is the
//! red channel.
//!
//! This function translates a 24-bit RGB color into a value that can be
//! written into the display's frame buffer in order to reproduce that color,
//! or the closest possible approximation of that color.
//!
//! \return Returns the display-driver specific color.
//
//*****************************************************************************
static uint32_t epd_grColorTranslate(const Graphics_Display *pvDisplayData,
                                     uint32_t ulValue) {
    //
    // Translate from a 24-bit RGB color to a color accepted by the LCD.
    //
    return(DPYCOLORTRANSLATE(ulValue));
}

//*****************************************************************************
//
//! Flushes any cached drawing operations.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This functions flushes any cached drawing operations to the display.  This
//! is useful when a local frame buffer is used for drawing operations, and the
//! flush would copy the local frame buffer to the display.
//!
//! \return None.
//
//*****************************************************************************
static void epd_grFlush(const Graphics_Display *pvDisplayData) {
    epd_phy_flush_buffer();
}

//*****************************************************************************
//
//! Send command to clear screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This function does a clear screen and the Display Buffer contents
//! are initialized to the current background color.
//!
//! \return None.
//
//*****************************************************************************
static void epd_grClearScreen(const Graphics_Display *pvDisplayData,
                              uint16_t ulValue) {
    // This fills the entire display to clear it
    // Some LCD drivers support a simple command to clear the display

    uint8_t init_byte = ulValue ? 0xff : 0x00;
    memset(epd_display_buffer, init_byte, sizeof(epd_display_buffer));
}

/// All the functions needed for grlib.
const Graphics_Display_Functions epd_grDisplayFunctions =
{
 .pfnPixelDraw = epd_grPixelDraw,
 .pfnPixelDrawMultiple = epd_grPixelDrawMultiple,
 .pfnLineDrawH = epd_grLineDrawH,
 .pfnLineDrawV = epd_grLineDrawV,
 .pfnRectFill = epd_grRectFill,
 .pfnColorTranslate = epd_grColorTranslate,
 .pfnFlush = epd_grFlush,
 .pfnClearDisplay = epd_grClearScreen
};

/// The driver definition for grlib.
Graphics_Display epd_gr_display_landscape = {
    .size = sizeof(Graphics_Display),
    .displayData = epd_display_buffer,
    .width = LCD_Y_SIZE,
    .heigth = LCD_X_SIZE, // heigth??? lol.
    .pFxns = &epd_grDisplayFunctions
};

Graphics_Display epd_gr_display_portrait = {
    .size = sizeof(Graphics_Display),
    .displayData = epd_display_buffer,
    .width = LCD_X_SIZE,
    .heigth = LCD_Y_SIZE, // heigth??? lol.
    .pFxns = &epd_grDisplayFunctions
};
