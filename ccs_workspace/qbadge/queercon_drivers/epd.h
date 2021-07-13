/*
 * epd_driver.h
 *
 *  Created on: May 6, 2019
 *      Author: george
 */

#ifndef QUEERCON_DRIVERS_EPD_H_
#define QUEERCON_DRIVERS_EPD_H_

#include <ti/grlib/grlib.h>

#include "qbadge_serial.h"
#include "../queercon_drivers/epd_phy.h"

//*****************************************************************************
// Number of pixels on LCD X-axis
#define LCD_X_SIZE EPD_WIDTH
// Number of pixels on LCD Y-axis
#define LCD_Y_SIZE EPD_HEIGHT
// Number of bits required to draw one pixel on the LCD screen
#define BPP 1

// Define LCD Screen Orientation Here
#define LANDSCAPE

//*****************************************************************************
//
// Defines for LCD driver configuration
//
//*****************************************************************************

/* Defines for pixels, colors, masks, etc. Anything qc12_oled.c needs*/


//*****************************************************************************
//
// This driver operates in four different screen orientations.
// If no screen orientation is selected, "landscape" mode will be used.
//
//*****************************************************************************
#if ! defined(LANDSCAPE) && ! defined(LANDSCAPE_FLIP) && \
    ! defined(PORTRAIT) && ! defined(PORTRAIT_FLIP)
#define LANDSCAPE
#endif

//*****************************************************************************
//
// Various definitions controlling coordinate space mapping and drawing
// direction in the four supported orientations.
//
//*****************************************************************************
#ifdef LANDSCAPE
#define MAPPED_X(x, y) (LCD_X_SIZE - (y) - 1)
#define MAPPED_Y(x, y) (x)
#define MAPPED_X_ROTATED(x, y) (y)
#define MAPPED_Y_ROTATED(x, y) (LCD_Y_SIZE - (x) - 1)
#define MAPPED_X_PORTRAIT(x, y) (LCD_X_SIZE - (x) - 1)
#define MAPPED_Y_PORTRAIT(x, y) (LCD_Y_SIZE - (y) - 1)
#endif
#ifdef PORTRAIT
#define MAPPED_X(x, y) (x)
#define MAPPED_Y(x, y) (y)
#endif
#ifdef LANDSCAPE_FLIP
#define MAPPED_X(x, y)  (y)
#define MAPPED_Y(x, y)  (x)
#endif
#ifdef PORTRAIT_FLIP
#define MAPPED_X(x, y)  (LCD_X_SIZE - (x) - 1)
#define MAPPED_Y(x, y)  (LCD_Y_SIZE - (y) - 1)
#endif


/// Translates a 24-bit RGB color to a display driver-specific color.
/// \return Returns BLACK if the input is black; WHITE otherwise.
#define DPYCOLORTRANSLATE(c) (c ? 1 : 0)

void epd_init();
extern Graphics_Display epd_gr_display_landscape;
extern Graphics_Display epd_gr_display_portrait;
extern const Graphics_Display_Functions epd_grDisplayFunctions;
void epd_flip();
extern uint8_t epd_upside_down;
extern uint8_t epd_do_partial;
#endif /* QUEERCON_DRIVERS_EPD_H_ */
