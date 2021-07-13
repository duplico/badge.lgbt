/*
 * graphics.c
 *
 *  qc16gr_drawImage and qc16gr_drawImageFromFile are derived from TI's
 *   GrLib, and are (c) Texas Instruments and (c) George Louthan
 *
 *  Remainder of file is (c) George Louthan 2019
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <grlib/grlib.h>
#include "graphics.h"
#include <spiffs.h>
#include <ti/sysbios/hal/Hwi.h>
#include <queercon_drivers/storage.h>

#define RLE_BATCH_READ_SIZE 38

/// More appropriate replacement function for Graphics_drawImage.
void qc16gr_drawImage(const Graphics_Context *context,
                      const Graphics_Image *bitmap,
                      int16_t x,
                      int16_t y)
{
    int16_t bPP, width, height, x0, x1, x2;
    const uint32_t palette[2] = {0, 1};
    const uint8_t *image;

    //
    // Check the arguments.
    //
    assert(context);
    assert(bitmap);

    //
    // Get the image format from the image data.
    //
    bPP = bitmap->bPP;

    assert((bPP & 0x0f) == 1);

    //
    // Get the image width from the image data.
    //
    width = bitmap->xSize;

    //
    // Get the image height from the image data.
    //
    height = bitmap->ySize;

    //
    // Return without doing anything if the entire image lies outside the
    // current clipping region.
    //
    if((x > context->clipRegion.xMax) ||
       ((x + width - 1) < context->clipRegion.xMin) ||
       (y > context->clipRegion.yMax) ||
       ((y + height - 1) < context->clipRegion.yMin))
    {
        return;
    }

    //
    // Get the starting X offset within the image based on the current clipping
    // region.
    //
    if(x < context->clipRegion.xMin)
    {
        x0 = context->clipRegion.xMin - x;
    }
    else
    {
        x0 = 0;
    }

    //
    // Get the ending X offset within the image based on the current clipping
    // region.
    //
    if((x + width - 1) > context->clipRegion.xMax)
    {
        x2 = context->clipRegion.xMax - x;
    }
    else
    {
        x2 = width - 1;
    }

    //
    // Reduce the height of the image, if required, based on the current
    // clipping region.
    //
    if((y + height - 1) > context->clipRegion.yMax)
    {
        height = context->clipRegion.yMax - y + 1;
    }

    //Check if palette is not valid
    if(!palette)
    {
        return;
    }

    //
    // Get the image pixels from the image data.
    //
    image = bitmap->pPixel;

    //
    // Check if the image is not compressed.
    //
    if(!(bPP & 0xF0))
    {
        //
        // The image is not compressed.  See if the top portion of the image
        // lies above the clipping region.
        //
        if(y < context->clipRegion.yMin)
        {
            //
            // Determine the number of rows that lie above the clipping region.
            //
            x1 = context->clipRegion.yMin - y;

            //
            // Skip past the data for the rows that lie above the clipping
            // region.
            //
            image += (((width * bPP) + 7) / 8) * x1;

            //
            // Decrement the image height by the number of skipped rows.
            //
            height -= x1;

            //
            // Increment the starting Y coordinate by the number of skipped
            // rows.
            //
            y += x1;
        }

        while(height--)
        {
            //
            // Draw this row of image pixels.
            //
            Graphics_drawMultiplePixelsOnDisplay(context->display, x + x0, y,
                                                 x0 & 7, x2 - x0 + 1, bPP,
                                                 image + ((x0 * bPP) / 8),
                                                 palette);

            //
            // Skip past the data for this row.
            //
            image += ((width * bPP) + 7) / 8;

            //
            // Increment the Y coordinate.
            //
            y++;
        }
    }
    else
    {
        //
        // The image is compressed with RLE4, RLE7 or RLE8 Algorithm
        //

        const uint8_t *pucData = image;
        uint8_t ucRunLength, rleType;
        uint16_t uiColor;

        rleType = (bPP >> 4) & 0x0F;
        bPP &= 0x0F;

        uint16_t x_offset = 0;
        uint16_t y_offset = 0;

        do {
            if(rleType == 8)   // RLE 8 bit encoding
            {
                // Read Run Length
                ucRunLength = *pucData++;
                // Read Color Pointer
                uiColor = *pucData++;
            }
            else if(rleType == 7)     // RLE 7 bit encoding
            {
                // Read Run Length
                ucRunLength = (*pucData) >> 1;
                // Read Color Pointer
                uiColor = (*pucData++) & 0x01;
            }
            else  // rleType = 4; RLE 4 bit encoding
            {
                // Read Run Length
                ucRunLength = (*pucData) >> 4;
                // Read Color Pointer
                uiColor = (*pucData++) & 0x0F;
            }
            uiColor = (uint16_t) palette[uiColor];

            // 0 = 1 pixel; 15 = 16, etc:
            ucRunLength++;

            while (ucRunLength--) {
                Graphics_drawPixelOnDisplay(context->display, x+x_offset, y+y_offset, uiColor);

                x_offset++;

                if (x_offset == width) {
                    x_offset = 0;
                    y_offset++;
                    if (y_offset == height) {
                        // done.
                        break;
                    }
                }
            }

        } while (y_offset < height);
    }
}

/// More appropriate replacement function for Graphics_drawImage, reading from SPIFFS.
void qc16gr_drawImageFromFile(const Graphics_Context *context,
                              char *pathname,
                              int16_t x,
                              int16_t y)
{
    Graphics_Image bitmap;
    volatile uint32_t keyHwi;

    int16_t bPP, width, height, x0, x1, x2;
    const uint32_t palette[2] = {0, 1};

    uint8_t *image;

    uint32_t img_size = 0;
    spiffs_stat stat;
    SPIFFS_stat(&fs, pathname, &stat);
    img_size = stat.size - sizeof(Graphics_Image);

    spiffs_file fd;
    fd = SPIFFS_open(&fs, pathname, SPIFFS_O_RDONLY, 0);
    int32_t ret = SPIFFS_read(&fs, fd, &bitmap, sizeof(Graphics_Image));

    if (ret < sizeof(Graphics_Image)) {
        SPIFFS_close(&fs, fd);
        storage_bad_file(pathname);
        return;
    }

    //
    // Check the arguments.
    //
    assert(context);

    //
    // Get the image format from the image data.
    //
    bPP = bitmap.bPP;

    assert((bPP & 0x0f) == 1);

    //
    // Get the image width from the image data.
    //
    width = bitmap.xSize;

    //
    // Get the image height from the image data.
    //
    height = bitmap.ySize;

    //
    // Return without doing anything if the entire image lies outside the
    // current clipping region.
    //
    if((x > context->clipRegion.xMax) ||
       ((x + width - 1) < context->clipRegion.xMin) ||
       (y > context->clipRegion.yMax) ||
       ((y + height - 1) < context->clipRegion.yMin))
    {
        return;
    }

    //
    // Get the starting X offset within the image based on the current clipping
    // region.
    //
    if(x < context->clipRegion.xMin)
    {
        x0 = context->clipRegion.xMin - x;
    }
    else
    {
        x0 = 0;
    }

    //
    // Get the ending X offset within the image based on the current clipping
    // region.
    //
    if((x + width - 1) > context->clipRegion.xMax)
    {
        x2 = context->clipRegion.xMax - x;
    }
    else
    {
        x2 = width - 1;
    }

    //
    // Reduce the height of the image, if required, based on the current
    // clipping region.
    //
    if((y + height - 1) > context->clipRegion.yMax)
    {
        height = context->clipRegion.yMax - y + 1;
    }

    //Check if palette is not valid
    if(!palette)
    {
        return;
    }

    //
    // Check if the image is not compressed.
    //
    if(!(bPP & 0xF0))
    {
        keyHwi = Hwi_disable();
        image = malloc(((width * bPP) + 7) / 8);
        Hwi_restore(keyHwi);
        //
        // The image is not compressed.  See if the top portion of the image
        // lies above the clipping region.
        //
        if(y < context->clipRegion.yMin)
        {
            //
            // Determine the number of rows that lie above the clipping region.
            //
            x1 = context->clipRegion.yMin - y;

            //
            // Skip past the data for the rows that lie above the clipping
            // region.
            //
            SPIFFS_lseek(&fs, fd, (((width * bPP) + 7) / 8) * x1, SPIFFS_SEEK_CUR);

            //
            // Decrement the image height by the number of skipped rows.
            //
            height -= x1;

            //
            // Increment the starting Y coordinate by the number of skipped
            // rows.
            //
            y += x1;
        }

        while(height--)
        {
            // Load an entire row (possibly outside of clipping area)
            ret = SPIFFS_read(&fs, fd, image, ((width * bPP) + 7) / 8);

            if (ret < ((width * bPP) + 7) / 8) {
                SPIFFS_close(&fs, fd);
                storage_bad_file(pathname);

                keyHwi = Hwi_disable();
                free(image);
                Hwi_restore(keyHwi);

                return;
            }

            // Draw the part of that row inside the clipping area:
            Graphics_drawMultiplePixelsOnDisplay(context->display, x + x0, y,
                                                 x0 & 7, x2 - x0 + 1, bPP,
                                                 image + ((x0 * bPP) / 8),
                                                 palette);

            //
            // Increment the Y coordinate.
            //
            y++;
        }
        keyHwi = Hwi_disable();
        free(image);
        Hwi_restore(keyHwi);
    } else {
        // The image is compressed with RLE4, RLE7 or RLE8 Algorithm
        keyHwi = Hwi_disable();
        image = malloc(RLE_BATCH_READ_SIZE);
        Hwi_restore(keyHwi);
        uint8_t read_len = 0;
        uint8_t img_index = read_len;

        uint8_t ucRunLength, rleType;
        uint16_t uiColor;

        rleType = (bPP >> 4) & 0x0F;
        bPP &= 0x0F;

        uint16_t x_offset = 0;
        uint16_t y_offset = 0;

        do {
            if (img_index == read_len) {
                if (img_size < RLE_BATCH_READ_SIZE) {
                    read_len = img_size;
                } else {
                    read_len = RLE_BATCH_READ_SIZE;
                }
                img_index = 0;
                ret = SPIFFS_read(&fs, fd, image, read_len);

                if (ret < read_len) {
                    SPIFFS_close(&fs, fd);
                    storage_bad_file(pathname);

                    keyHwi = Hwi_disable();
                    free(image);
                    Hwi_restore(keyHwi);

                    return;
                }
            }

            if(rleType == 8)   // RLE 8 bit encoding
            {
                // Read Run Length
                ucRunLength = image[img_index++];
                // Read Color
                uiColor = image[img_index++];
            }
            else if(rleType == 7)     // RLE 7 bit encoding
            {
                // Read Run Length
                ucRunLength = image[img_index] >> 1;
                // Read Color Pointer
                uiColor = image[img_index++] & 0x01;
            }
            else  // rleType = 4; RLE 4 bit encoding
            {
                // Read Run Length
                ucRunLength = image[img_index] >> 4;
                // Read Color Pointer
                uiColor = image[img_index++] & 0x0F;
            }
            uiColor = (uint16_t) palette[uiColor];

            // 0 = 1 pixel; 15 = 16, etc:
            ucRunLength++;

            while (ucRunLength--) {
                Graphics_drawPixelOnDisplay(context->display, x+x_offset, y+y_offset, uiColor);

                x_offset++;

                if (x_offset == width) {
                    x_offset = 0;
                    y_offset++;
                    if (y_offset == height) {
                        // done.
                        break;
                    }
                }
            }

        } while (y_offset < height);

        keyHwi = Hwi_disable();
        free(image);
        Hwi_restore(keyHwi);
    }
    SPIFFS_close(&fs, fd);
}

uint16_t qc16gr_get_image_size(const Graphics_Image *bitmap)
{
    int16_t bPP, width, height;
    const uint8_t *image;

    //
    // Check the arguments.
    //
    assert(bitmap);

    //
    // Get the image format from the image data.
    //
    bPP = bitmap->bPP;

    assert((bPP & 0x0f) == 1);

    width = bitmap->xSize;
    height = bitmap->ySize;

    //
    // Get the image pixels from the image data.
    //
    image = bitmap->pPixel;

    if(!(bPP & 0xF0))
    {
        if (width % 8) {
            return height * (width+1) / 8;
        } else {
            return (height * width) / 8;
        }
    }
    else
    {
        //
        // The image is compressed with RLE4, RLE7 or RLE8 Algorithm
        //

        uint16_t img_size = 0;

        const uint8_t *pucData = image;
        uint8_t ucRunLength, rleType;

        rleType = (bPP >> 4) & 0x0F;
        bPP &= 0x0F;

        uint16_t x_offset = 0;
        uint16_t y_offset = 0;

        do {
            if(rleType == 8)   // RLE 8 bit encoding
            {
                // Read Run Length
                ucRunLength = *pucData++;
            }
            else if(rleType == 7)     // RLE 7 bit encoding
            {
                // Read Run Length
                ucRunLength = (*pucData++) >> 1;
            }
            else  // rleType = 4; RLE 4 bit encoding
            {
                // Read Run Length
                ucRunLength = (*pucData++) >> 4;
            }
            img_size++;
            // 0 = 1 pixel; 15 = 16, etc:
            ucRunLength++;

            while (ucRunLength--) {
                x_offset++;
                if (x_offset == width) {
                    x_offset = 0;
                    y_offset++;
                    if (y_offset == height) {
                        // done.
                        break;
                    }
                }
            }

        } while (y_offset < height);
        return img_size;
    }
}

void fadeRectangle(Graphics_Context *gr_context, Graphics_Rectangle *rect) {
    uint32_t fg_prev;
    fg_prev = gr_context->foreground;
    gr_context->foreground = gr_context->background;
    for (uint16_t x=rect->xMin; x<=rect->xMax; x++) {
        for (uint16_t y=rect->yMin+x%2; y<=rect->yMax; y+=2) {
            Graphics_drawPixel(gr_context, x, y);
        }
    }
    gr_context->foreground = fg_prev;
}

void fadeRectangle_xy(Graphics_Context *gr_context, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    Graphics_Rectangle rect;
    rect = (Graphics_Rectangle) {
        x0, y0,
        x1, y1,
    };

    fadeRectangle(gr_context, &rect);
}

void fillRectangle(Graphics_Context *gr_context, Graphics_Rectangle *rect) {
    for (uint16_t y=rect->yMin; y<=rect->yMax; y++) {
        Graphics_drawLineH(gr_context, rect->xMin, rect->xMax, y);
    }
}

void drawRectangle_xy(Graphics_Context *gr_context, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    Graphics_Rectangle rect;
    rect = (Graphics_Rectangle) {
        x0, y0,
        x1, y1,
    };

    Graphics_drawRectangle(gr_context, &rect);
}

void fillRectangle_xy(Graphics_Context *gr_context, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    Graphics_Rectangle rect;
    rect = (Graphics_Rectangle) {
        x0, y0,
        x1, y1,
    };

    fillRectangle(gr_context, &rect);
}

void qc16gr_drawProgressBar(Graphics_Context *gr_context, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t progress, uint32_t total) {
    drawRectangle_xy(gr_context, x, y, x+width-1, y+height-1);
    fillRectangle_xy(gr_context, x, y, x+(width*progress/total), y+height-1);
}

void qc16gr_draw_larrow(Graphics_Context *gr_context, uint16_t x, uint16_t y, uint16_t height) {
    for (uint8_t i=0; i<height/2; i++) {
        Graphics_drawLineH(gr_context, x - height/2 + i, x, y-i);
        Graphics_drawLineH(gr_context, x - height/2 + i, x, y+i);
    }
}

void qc16gr_draw_rarrow(Graphics_Context *gr_context, uint16_t x, uint16_t y, uint16_t height) {
    for (uint8_t i=0; i<height/2; i++) {
        Graphics_drawLineH(gr_context, x, x + height/2 - i, y-i);
        Graphics_drawLineH(gr_context, x, x + height/2 - i, y+i);
    }
}
